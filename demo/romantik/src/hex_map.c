#include "hex_map.h"
#include "hex.h"

#include <core/math.h>
#include <core/memory.h>
#include <core/macro.h>

void hex_map_init(HexMap *map, u32 hex_size, u32 horizontal_grids, u32 vertical_grids)
{
    map->grid_width  = walrus_min(MAX_GRIDS_HORIZONTAL, horizontal_grids);
    map->grid_height = walrus_min(MAX_GRIDS_VERTICAL, vertical_grids);

    map->hex_size = hex_size;

    map->grids = walrus_new0(HexGrid, map->grid_width * map->grid_height);
}

static u32 get_index(HexMap *map, i32 q, i32 r)
{
    u32 const center_q = ceil((map->grid_width - 1) / 2.0);
    u32 const center_r = ceil((map->grid_height - 1) / 2.0);
    q += center_q;
    r += center_r;
    if (q >= 0 && r >= 0) {
        q = walrus_min((u32)q, map->grid_width - 1);
        r = walrus_min((u32)r, map->grid_height - 1);
        return r * map->grid_width + q;
    }
    return 0;
}

bool hex_map_check_in_bound(HexMap *map, i32 q, i32 r)
{
    u32 const center_q = ceil((map->grid_width - 1) / 2.0);
    u32 const center_r = ceil((map->grid_height - 1) / 2.0);

    q += center_q;
    r += center_r;
    if (q >= 0 && r >= 0) {
        return (u32)q < map->grid_width && (u32)r < map->grid_height;
    }
    return false;
}

bool hex_map_test_flags(HexMap *map, i32 q, i32 r, u32 flags)
{
    if (hex_map_check_in_bound(map, q, r)) {
        return map->grids[get_index(map, q, r)].flags & flags;
    }
    return false;
}

bool hex_map_set_flags(HexMap *map, i32 q, i32 r, u32 flags)
{
    if (hex_map_check_in_bound(map, q, r)) {
        map->grids[get_index(map, q, r)].flags = flags;
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

bool hex_map_connect_border(HexMap *map, i32 q, i32 r)
{
    if (hex_map_check_in_bound(map, q, r)) {
        vec2 const dirs[] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
        HexGrid   *center = &map->grids[get_index(map, q, r)];
        for (i32 i = 0; i < 6; ++i) {
            i32 n_q = q + dirs[i][0];
            i32 n_r = r + dirs[i][1];
            if (!hex_map_check_in_bound(map, n_q, n_r)) {
                continue;
            }

            HexGrid *neighbor = &map->grids[get_index(map, n_q, n_r)];

            i32 const neighbor_i = (i + 3) % 6;
            if (neighbor->flags == center->flags) {
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

void hex_map_compute_model_pixel(HexMap *map, mat4 model, f32 x, f32 y)
{
    i32 q, r;
    hex_pixel_to_qr(map->hex_size, x, y, &q, &r);
    hex_map_compute_model(map, model, q, r);
}

void hex_map_compute_model(HexMap *map, mat4 model, i32 q, i32 r)
{
    f32      x, z;
    HexGrid *grid = hex_map_get_grid(map, q, r);
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

u32 hex_map_compute_instance_buffer(HexMap *map, HexInstanceBuffer *buffers, u64 max_size, u32 flags)
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
        if (map->grids[i].flags & flags) {
            hex_map_compute_model(map, buffers[cnt].model, q - center_q, r - center_r);
            buffers[cnt].layer[0] = hex_map_get_border_type(map->grids[i].abs_border);
            ++cnt;
        }
    }
    return cnt;
}

HexGrid *hex_map_get_grid(HexMap *map, i32 q, i32 r)
{
    return &map->grids[get_index(map, q, r)];
}

u8 hex_map_get_border_type(u8 bits)
{
    // clang-format off
    u8 const border_layer_map[]= {
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
    u8 type = 0;
    for (u32 i = 0; i < walrus_array_len(border_layer_map); ++i) {
        if (border_layer_map[i] == bits) {
            type = i;
            break;
        }
    }
    return type;
}
