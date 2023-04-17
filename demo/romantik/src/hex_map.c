#include "hex_map.h"
#include "hex.h"

#include <core/math.h>
#include <core/memory.h>

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

void hex_map_compute_model_pixel(HexMap *map, mat4 model, f32 x, f32 y)
{
    i32 q, r;
    hex_pixel_to_qr(map->hex_size, x, y, &q, &r);
    hex_map_compute_model(map, model, q, r);
}

void hex_map_compute_model(HexMap *map, mat4 model, i32 q, i32 r)
{
    f32 x, z;
    hex_qr_to_pixel(map->hex_size, q, r, &x, &z);
    glm_mat4_identity(model);
    glm_translate(model, (vec3){x, 0, z});
    glm_scale(model, (vec3){map->hex_size, map->hex_size, map->hex_size});
}

u32 hex_map_compute_models(HexMap *map, mat4 *models, u64 max_size, u32 flags)
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
            hex_map_compute_model(map, models[cnt], q - center_q, r - center_r);
            ++cnt;
        }
    }
    return cnt;
}
