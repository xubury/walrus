#pragma once

#include <core/type.h>
#include <cglm/cglm.h>

#define MAX_GRIDS_HORIZONTAL 100
#define MAX_GRIDS_VERTICAL   100

typedef struct {
    void *grids;
    u32   grid_width;
    u32   grid_height;
    u32   hex_size;
} HexMap;

void hex_map_init(HexMap *map, u32 hex_size, u64 data_size, u32 horizontal_grids, u32 vertical_grids);

bool hex_map_check_in_bound(HexMap *map, i32 q, i32 r);

u32 hex_map_get_index(HexMap *map, i32 q, i32 r);
