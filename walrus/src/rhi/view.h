#pragma once

#include <core/type.h>

#include <cglm/types.h>

typedef struct {
    i64 x;
    i64 y;

    u64 width;
    u64 height;
} ViewRect;

typedef struct {
    u8  index[8];
    f32 depth;
    u8  stencil;
    u16 flags;
} RenderClear;

typedef struct {
    RenderClear clear;
    ViewRect    viewport;

    mat4 view;
    mat4 projection;
} RenderView;
