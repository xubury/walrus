#pragma once

#include <rhi/rhi_defines.h>
#include <rhi/type.h>

#include "uniform_buffer.h"
#include "view.h"

typedef struct {
    u32 width;
    u32 height;
} Resolution;

typedef struct {
    Walrus_BufferHandle       handle;
    Walrus_VertexLayoutHandle layout_handle;
    u32                       offset;
} VertexStream;

typedef struct {
    VertexStream streams[WR_RHI_MAX_VERTEX_STREAM];
    u16          stream_mask;

    u32 num_vertices;
    u32 num_indices;

    u32 uniform_begin;
    u32 uniform_end;

    u32 start_matrix;
    u32 num_matrices;
} RenderDraw;

typedef struct {
    u32 num_x;
    u32 num_y;
    u32 num_z;

    u32 uniform_begin;
    u32 uniform_end;
} RenderCompute;

typedef struct {
    RenderDraw    draw;
    RenderCompute compute;
} RenderItem;

typedef struct {
    RenderView views[WR_RHI_MAX_VIEWS];

    u32        num_render_items;
    RenderItem render_items[WR_RHI_MAX_DRAW_CALLS];

    u16                  view_ids[WR_RHI_MAX_DRAW_CALLS];
    Walrus_ProgramHandle program[WR_RHI_MAX_DRAW_CALLS];

    Resolution resolution;

    UniformBuffer *uniforms;

    mat4 matrix_cache[WR_RHI_MAX_MATRIX_CACHE];
    u32  num_matrices;
} RenderFrame;

void frame_init(RenderFrame *frame);

void frame_shutdown(RenderFrame *frame);

void frame_start(RenderFrame *frame);

void frame_finish(RenderFrame *frame);

u32 frame_add_matrices(RenderFrame *frame, mat4 const mat, u32 *num);

void draw_clear(RenderDraw *draw, u8 flags);
