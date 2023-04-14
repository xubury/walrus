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
    Walrus_BufferHandle handle;
    Walrus_LayoutHandle layout_handle;
    u64                 offset;
} VertexStream;

typedef struct {
    VertexStream streams[WR_RHI_MAX_VERTEX_STREAM];
    u16          stream_mask;

    u32 num_vertices;
    u32 num_indices;

    u32 uniform_begin;
    u32 uniform_end;

    Walrus_BufferHandle index_buffer;
    u8                  index_size;
    u64                 index_offset;

    uint32_t            instance_offset;
    uint32_t            num_instances;
    Walrus_BufferHandle instance_buffer;
    Walrus_LayoutHandle instance_layout;

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
    u32 sampler_flags;
    enum {
        WR_RHI_BIND_TEXTURE,
        WR_RHI_BIND_IMAGE,

        WR_RHI_BIND_COUNT
    } type;
    Walrus_Handle id;
    u8            format;
    u8            access;
    u8            mip;
} Binding;

typedef struct {
    Binding bindings[WR_RHI_MAX_TEXTURE_SAMPLERS];
} RenderBind;

typedef struct {
    RenderDraw    draw;
    RenderCompute compute;
} RenderItem;

typedef struct {
    RenderView views[WR_RHI_MAX_VIEWS];

    u32        num_render_items;
    RenderItem render_items[WR_RHI_MAX_DRAW_CALLS];
    RenderBind render_binds[WR_RHI_MAX_DRAW_CALLS];

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

void bind_clear(RenderBind *bind, u8 flags);
