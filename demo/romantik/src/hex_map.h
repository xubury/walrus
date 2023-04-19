#pragma once

#include <core/type.h>
#include <cglm/cglm.h>

#define MAX_GRIDS_HORIZONTAL 100
#define MAX_GRIDS_VERTICAL   100

typedef struct {
    u8  border;
    u32 flags;
    u8  abs_border;
    u8  shift_bits;
} HexGrid;

typedef struct {
    HexGrid *grids;
    u32      grid_width;
    u32      grid_height;
    u32      hex_size;
} HexMap;

typedef struct {
    vec4 layer;
    mat4 model;
} HexInstanceBuffer;

void hex_map_init(HexMap *map, u32 hex_size, u32 horizontal_grids, u32 vertical_grids);

bool hex_map_check_in_bound(HexMap *map, i32 q, i32 r);

bool hex_map_test_flags(HexMap *map, i32 q, i32 r, u32 flags);

bool hex_map_set_flags(HexMap *map, i32 q, i32 r, u32 flags);

bool hex_map_connect_border(HexMap *map, i32 q, i32 r);

void hex_map_compute_model(HexMap *map, mat4 model, i32 q, i32 r);

void hex_map_compute_model_pixel(HexMap *map, mat4 model, f32 x, f32 y);

u32 hex_map_compute_instance_buffer(HexMap *map, HexInstanceBuffer *buffers, u64 max_size, u32 flags);

HexGrid *hex_map_get_grid(HexMap *map, i32 q, i32 r);
