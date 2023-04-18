#pragma once

#include "hex_map.h"
typedef enum {
    TERRAIN_GRASSLAND,
    TERRAIN_WATER,
    TERRAIN_SAND,
    TERRAIN_SNOW,
    TERRAIN_COUNT,
} Terrain;

typedef enum {
    HEX_FLAG_NONE      = 0,
    HEX_FLAG_GRASSLAND = 1 << TERRAIN_GRASSLAND,
    HEX_FLAG_WATER     = 1 << TERRAIN_WATER,
    HEX_FLAG_SAND      = 1 << TERRAIN_SAND,
    HEX_FLAG_SNOW      = 1 << TERRAIN_SNOW,
    HEX_FLAG_AVAIL     = 1 << TERRAIN_COUNT,
    HEX_FLAG_PLACED    = HEX_FLAG_GRASSLAND | HEX_FLAG_WATER | HEX_FLAG_SAND | HEX_FLAG_SNOW,
} HexFlag;

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
