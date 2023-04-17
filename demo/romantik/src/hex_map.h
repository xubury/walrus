#pragma once

#include <core/type.h>
#include <cglm/cglm.h>

#define MAX_GRIDS_HORIZONTAL 100
#define MAX_GRIDS_VERTICAL   100

typedef enum {
    HEX_FLAG_NONE   = 0,
    HEX_FLAG_NORMAL = 1 << 0,
} HexFlag;

typedef struct {
    u32 id;
    u32 flags;
} HexGrid;

typedef struct {
    HexGrid *grids;
    u32      grid_width;
    u32      grid_height;
    u32      hex_size;
} HexMap;

void hex_map_init(HexMap *map, u32 hex_size, u32 horizontal_grids, u32 vertical_grids);

bool hex_map_test_flags(HexMap *map, i32 q, i32 r, u32 flags);

bool hex_map_set_flags(HexMap *map, i32 q, i32 r, u32 flags);

void hex_map_compute_model(HexMap *map, mat4 model, i32 q, i32 r);

void hex_map_compute_model_pixel(HexMap *map, mat4 model, f32 x, f32 y);

u32 hex_map_compute_models(HexMap *map, mat4 *models, u64 max_size, u32 flags);
