#pragma once

#include "hex_map.h"

typedef enum {
    TERRAIN_GRASSLAND,
    TERRAIN_WATER,
    TERRAIN_SAND,
    TERRAIN_SNOW,
    TERRAIN_COUNT,
} Terrain;

#define romantik_terrain_flag(type) (1 << (type))

typedef enum {
    HEX_FLAG_NONE   = 0,
    HEX_FLAG_PLACED = romantik_terrain_flag(TERRAIN_GRASSLAND) | romantik_terrain_flag(TERRAIN_WATER) |
                      romantik_terrain_flag(TERRAIN_SAND) | romantik_terrain_flag(TERRAIN_SNOW),
    HEX_FLAG_AVAIL = romantik_terrain_flag(TERRAIN_COUNT),
} HexFlag;

typedef struct {
    u8  border;
    u64 flags;
    u8  abs_border;
    u8  shift_bits;
} HexGrid;

typedef struct {
    vec4 border;
    mat4 model;
} HexInstanceBuffer;

typedef struct {
    HexMap map;

    Terrain next_terrain;

    u32 score;
    u32 num_placed_grids;
    u32 num_avail_grids;
} Romantik_Game;

void romantik_game_init(Romantik_Game *game);

bool romantik_place_grid(Romantik_Game *game, i32 q, i32 r);

bool romantik_set_avail(Romantik_Game *game, i32 q, i32 r);

HexGrid *romantik_get_grid(HexMap *map, i32 q, i32 r);

u32 romantik_compute_instance_buffer(HexMap *map, HexInstanceBuffer *buffers, u64 max_size, u64 flags);

u8 romantik_get_border_type(u8 bits);

bool romantik_test_flags(HexMap *map, i32 q, i32 r, u64 flags);

bool romantik_set_flags(HexMap *map, i32 q, i32 r, u64 flags);

bool romantik_connect_border(HexMap *map, i32 q, i32 r);

void romantik_compute_model(HexMap *map, mat4 model, i32 q, i32 r);

void romantik_compute_model_pixel(HexMap *map, mat4 model, f32 x, f32 y);
