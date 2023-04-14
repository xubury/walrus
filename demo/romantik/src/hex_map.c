#include "hex_map.h"
#include "hex.h"

#include <core/math.h>
#include <core/memory.h>

void hex_map_init(HexMap *map, u32 hex_size, u32 horizontal_grids, u32 vertical_grids)
{
    map->grid_width  = walrus_min(MAX_GRIDS_HORIZONTAL, horizontal_grids);
    map->grid_height = walrus_min(MAX_GRIDS_VERTICAL, vertical_grids);

    map->hex_size = hex_size;
    map->grids    = walrus_new0(HexGrid, map->grid_width * map->grid_height);
}

void hex_map_set_flags(HexMap *map, i32 q, i32 r, u32 flags)
{
    i32 const center_q = map->grid_width / 2.0;
    i32 const center_r = map->grid_height / 2.0;
    u32       q_offset = q + center_q;
    u32       r_offset = r + center_r;

    q_offset = walrus_clamp(q_offset, 0, map->grid_width);
    r_offset = walrus_clamp(r_offset, 0, map->grid_height);

    map->grids[r_offset * map->grid_width + q_offset].flags = flags;
}
void hex_map_compute_model_pixel(HexMap *map, mat4 model, f32 x, f32 y)
{
    i32 q, r;
    hex_pixel_to_qr(map->hex_size, x, y, &q, &r);
    hex_qr_to_pixel(map->hex_size, q, r, &x, &y);
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

    u32 const center_q = map->grid_width / 2.0;
    u32 const center_r = map->grid_height / 2.0;
    for (u64 i = 0; i < size && cnt < max_size; ++i) {
        if (map->grids[i].flags & flags) {
            i32 r = i / map->grid_width - center_q;
            i32 q = i % map->grid_width - center_r;
            hex_map_compute_model(map, models[cnt], q, r);
            ++cnt;
        }
    }
    return cnt;
}
