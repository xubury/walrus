#pragma once

#include <core/rect.h>

#include <cglm/types.h>

typedef struct {
    u8  index[8];
    f32 depth;
    u8  stencil;
    u16 flags;
} Walrus_RenderClear;

typedef struct {
    Walrus_RenderClear clear;
    Walrus_Rect        viewport;

    mat4 view;
    mat4 projection;
} RenderView;
