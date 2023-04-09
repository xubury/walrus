#pragma once

#include <rhi/rhi_defines.h>
#include <rhi/type.h>

#include "view.h"

typedef struct {
    u32 width;
    u32 height;
} Walrus_Resolution;

typedef struct {
    u32 offset;
} Walrus_VertexStream;

typedef struct {
    Walrus_VertexStream stream[WR_RHI_MAX_VERTEX_STREAM];
} Walrus_RenderDraw;

typedef struct {
    u32 num_x;
    u32 num_y;
    u32 num_z;
} Walrus_RenderCompute;

typedef struct {
    Walrus_RenderDraw    draw;
    Walrus_RenderCompute compute;
} Walrus_RenderItem;

typedef struct {
    u16               num_views;
    Walrus_RenderView views[WR_RHI_MAX_VIEWS];

    u16               num_render_items;
    Walrus_RenderItem render_items[WR_RHI_MAX_DRAW_CALLS];

    u16                  view_ids[WR_RHI_MAX_DRAW_CALLS];
    Walrus_ProgramHandle program[WR_RHI_MAX_DRAW_CALLS];

    Walrus_Resolution resolution;
} Walrus_RenderFrame;
