#pragma once

#include <core/rect.h>

typedef struct {
    u8  index[8];
    f32 depth;
    u8  stencil;
    u16 flags;
} Walrus_RenderClear;

typedef struct {
    Walrus_RenderClear clear;
    Walrus_Rect        viewport;
} Walrus_RenderView;
