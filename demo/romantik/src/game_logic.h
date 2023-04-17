#pragma once

#include "hex_map.h"

typedef enum {
    HEX_FLAG_NONE   = 0,
    HEX_FLAG_AVAIL  = 1 << 0,
    HEX_FLAG_PLACED = 1 << 1,
} HexFlag;

typedef struct {
    HexMap map;

    u32 score;
    u32 num_placed_grids;
    u32 num_avail_grids;
} RomantikGame;

void romantik_game_init(RomantikGame *game);

bool romantik_place_grid(RomantikGame *game, i32 q, i32 r);

bool romantik_set_avail(RomantikGame *game, i32 q, i32 r);
