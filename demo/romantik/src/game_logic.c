#include "game_logic.h"
#include "hex.h"

#include <core/assert.h>
#include <core/log.h>
#include <core/math.h>

void romantik_game_init(Romantik_Game *game)
{
    hex_map_init(&game->map, 2, sizeof(HexGrid), 10, 10);

    game->next_terrain     = TERRAIN_GRASSLAND;
    game->score            = 0;
    game->num_placed_grids = 0;
    game->num_avail_grids  = 0;
}

static void set_neighbor_avails(Romantik_Game *game, i32 q, i32 r)
{
    vec2 const dirs[] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
    for (u8 i = 0; i < 6; ++i) {
        i32 n_q = q + dirs[i][0];
        i32 n_r = r + dirs[i][1];
        romantik_set_avail(game, n_q, n_r);
    }
}

HexGrid *romantik_get_grid(HexMap *map, i32 q, i32 r)
{
    return &((HexGrid *)map->grids)[hex_map_get_index(map, q, r)];
}

bool romantik_place_grid(Romantik_Game *game, i32 q, i32 r)
{
    if (romantik_test_flags(&game->map, q, r, HEX_FLAG_AVAIL)) {
        romantik_set_flags(&game->map, q, r, romantik_terrain_flag(game->next_terrain));
        ++game->num_placed_grids;
        --game->num_avail_grids;
        set_neighbor_avails(game, q, r);
        romantik_connect_border(&game->map, q, r);
        return true;
    }
    return false;
}

bool romantik_set_avail(Romantik_Game *game, i32 q, i32 r)
{
    if (hex_map_check_in_bound(&game->map, q, r) && !romantik_test_flags(&game->map, q, r, HEX_FLAG_PLACED) &&
        !romantik_test_flags(&game->map, q, r, HEX_FLAG_AVAIL)) {
        romantik_set_flags(&game->map, q, r, HEX_FLAG_AVAIL);
        ++game->num_avail_grids;
        return true;
    }
    return false;
}

bool romantik_test_flags(HexMap *map, i32 q, i32 r, u64 flags)
{
    if (hex_map_check_in_bound(map, q, r)) {
        return romantik_get_grid(map, q, r)->flags & flags;
    }
    return false;
}

bool romantik_set_flags(HexMap *map, i32 q, i32 r, u64 flags)
{
    if (hex_map_check_in_bound(map, q, r)) {
        romantik_get_grid(map, q, r)->flags = flags;
        return true;
    }
    return false;
}

static void compute_abs_border(HexGrid *grid)
{
    u8 shift_bits = 0;
    u8 min_border = grid->border;
    u8 border     = grid->border;
    for (u8 i = 0; i < 5; ++i) {
        border = ((border << 5) & 0x3f) | ((border >> 1) & 0x3f);
        if (border < min_border) {
            min_border = border;
            shift_bits = i + 1;
        }
    }
    grid->abs_border = min_border;
    grid->shift_bits = shift_bits;
}

bool romantik_connect_border(HexMap *map, i32 q, i32 r)
{
    if (hex_map_check_in_bound(map, q, r)) {
        vec2 const dirs[] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
        HexGrid   *center = romantik_get_grid(map, q, r);
        for (i32 i = 0; i < 6; ++i) {
            i32 n_q = q + dirs[i][0];
            i32 n_r = r + dirs[i][1];
            if (!hex_map_check_in_bound(map, n_q, n_r)) {
                continue;
            }

            HexGrid *neighbor = romantik_get_grid(map, n_q, n_r);

            i32 const neighbor_i = (i + 3) % 6;
            if (neighbor->flags & center->flags) {
                walrus_assert((center->border & (1 << i)) == 0);
                walrus_assert((neighbor->border & (1 << neighbor_i)) == 0);
                center->border |= (1 << i);
                neighbor->border |= (1 << neighbor_i);
                compute_abs_border(neighbor);
            }
        }
        compute_abs_border(center);
        return true;
    }
    return false;
}

void romantik_compute_model_pixel(HexMap *map, mat4 model, f32 x, f32 y)
{
    i32 q, r;
    hex_pixel_to_qr(map->hex_size, x, y, &q, &r);
    romantik_compute_model(map, model, q, r);
}

void romantik_compute_model(HexMap *map, mat4 model, i32 q, i32 r)
{
    f32      x, z;
    HexGrid *grid = romantik_get_grid(map, q, r);
    hex_qr_to_pixel(map->hex_size, q, r, &x, &z);
    mat4 rot   = GLM_MAT4_IDENTITY_INIT;
    mat4 trans = GLM_MAT4_IDENTITY_INIT;
    mat4 scale = GLM_MAT4_IDENTITY_INIT;
    glm_scale(scale, (vec3){map->hex_size, map->hex_size, map->hex_size});
    glm_rotate(rot, glm_rad(60 * grid->shift_bits), (vec3){0, 1, 0});
    glm_translate(trans, (vec3){x, 0, z});

    glm_mul(rot, scale, model);
    glm_mul(trans, model, model);
}

u32 romantik_compute_instance_buffer(HexMap *map, HexInstanceBuffer *buffers, u64 max_size, u64 flags)
{
    u64 size = map->grid_width * map->grid_height;
    u64 cnt  = 0;

    u32 const center_q = ceil((map->grid_width - 1) / 2.0);
    u32 const center_r = ceil((map->grid_height - 1) / 2.0);
    for (u64 i = 0; i < size && cnt < max_size; ++i) {
        u32 q = i % map->grid_width;
        u32 r = i / map->grid_width;
        if (r >= map->grid_height) {
            continue;
        }
        HexGrid *grid = &((HexGrid *)map->grids)[i];
        if (grid->flags & flags) {
            romantik_compute_model(map, buffers[cnt].model, q - center_q, r - center_r);
            romantik_get_terrain_color(walrus_u32cnttz(grid->flags), buffers[cnt].color);
            buffers[cnt].border_type = romantik_get_border_type(grid->abs_border);
            ++cnt;
        }
    }
    return cnt;
}
// clang-format off
static u8 const border_layer_map[] = {
    0x0,
    0x1,

    0x3, //0b11
    0x5, //0b101
    0x9, //0b1001

    0x7, //0b111
    0xb, //0b1011
    0xd, //0b1101
    0x15,//0b10101

    0xf, //0b1111
    0x17,//0b10111
    0x1b,//0b11011

    0x1f,// 0b11111
    0x3f,// 0b111111
} ;
// clang-format on

u8 romantik_get_border_type(u8 bits)
{
    u8 type = 0;
    for (u32 i = 0; i < walrus_array_len(border_layer_map); ++i) {
        if (border_layer_map[i] == bits) {
            type = i;
            break;
        }
    }
    return type;
}

static vec3 colors[TERRAIN_COUNT] = {
    {0.529411, 0.9, 0.556862}, {0.529411, 0.756862, 1.0}, {1.0, 0.756862, 0.529411}, {0.756862, 0.756862, 0.756862}};

void romantik_get_terrain_color(Terrain terrain, vec4 color)
{
    glm_vec3_copy(colors[walrus_clamp(terrain, 0, TERRAIN_COUNT - 1)], color);
}
