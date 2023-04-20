#include "hex_map.h"

#include <core/math.h>
#include <core/memory.h>

void hex_map_init(HexMap *map, u32 hex_size, u64 data_size, u32 horizontal_grids, u32 vertical_grids)
{
    map->grid_width  = walrus_min(MAX_GRIDS_HORIZONTAL, horizontal_grids);
    map->grid_height = walrus_min(MAX_GRIDS_VERTICAL, vertical_grids);

    map->hex_size = hex_size;

    map->grids = walrus_malloc0(data_size * map->grid_width * map->grid_height);
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

u32 hex_map_get_index(HexMap *map, i32 q, i32 r)
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
