#pragma once

#include <core/type.h>
#include <core/macro.h>
#include <core/math.h>
#include <rhi/type.h>

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
    ViewRect    scissor;

    mat4 view;
    mat4 projection;

    Walrus_ViewMode mode;
} RenderView;

WR_INLINE void viewrect_intersect(ViewRect *rect, ViewRect const *other)
{
    u16 const sx = walrus_max(other->x, rect->x);
    u16 const sy = walrus_max(other->y, rect->y);
    u16 const ex = walrus_min(other->x + other->width, rect->x + rect->width);
    u16 const ey = walrus_min(other->y + other->height, rect->y + rect->height);
    rect->x      = sx;
    rect->y      = sy;
    rect->width  = walrus_u32satsub(ex, sx);
    rect->height = walrus_u32satsub(ey, sy);
}

WR_INLINE bool viewrect_zero_area(ViewRect const *rect)
{
    return rect->width == 0 || rect->height == 0;
}

WR_INLINE bool viewrect_equal(ViewRect const *rect1, ViewRect const *rect2)
{
    return rect1->x == rect2->x && rect1->y == rect2->y && rect1->width == rect2->width &&
           rect1->height == rect2->height;
}
