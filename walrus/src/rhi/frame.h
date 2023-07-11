#pragma once

#include <rhi/rhi_defines.h>
#include <rhi/type.h>

#include "uniform_buffer.h"
#include "view.h"
#include "command_buffer.h"

#define FREE_HANDLE(max, name)    \
    struct {                      \
        u32           num;        \
        Walrus_Handle queue[max]; \
    } queue_##name

typedef struct {
    u32                  depth;
    u8                   blend;
    u16                  view_id;
    Walrus_ProgramHandle program;
    u32                  sequence;
} Sortkey;

typedef enum {
    SORT_PROGRAM,
    SORT_DEPTH,
    SORT_SEQUENCE
} SortKeyType;

u64  sortkey_encode_draw(Sortkey *key, SortKeyType type);
u64  sortkey_encode_compute(Sortkey *key);
void sortkey_reset(Sortkey *key);
bool sortkey_decode(Sortkey *key, u64 key_val, u16 *view_map);
u16  sortkey_decode_view(u64 key_val);
u64  sortkey_remapview(u64 key_val, u16 *view_map);

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

    u32                 instance_offset;
    u32                 num_instances;
    Walrus_BufferHandle instance_buffer;
    Walrus_LayoutHandle instance_layout;

    u64 stencil;
    u64 state_flags;
    u32 blend_factor;

    u32 start_matrix;
    u32 num_matrices;

    ViewRect scissor;
} RenderDraw;

typedef struct {
    u32 num_x;
    u32 num_y;
    u32 num_z;

    u32 uniform_begin;
    u32 uniform_end;

    u32 start_matrix;
    u32 num_matrices;
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
    Walrus_BufferHandle handle;
    u32                 offset;
    u32                 size;
} BlockBinding;

typedef struct {
    Binding      bindings[WR_RHI_MAX_TEXTURE_SAMPLERS];
    BlockBinding block_bindings[WR_RHI_MAX_UNIFORM_BINDINGS];
} RenderBind;

typedef struct {
    RenderDraw    draw;
    RenderCompute compute;
} RenderItem;

typedef struct {
    CommandBuffer cmd_pre;
    CommandBuffer cmd_post;

    RenderView views[WR_RHI_MAX_VIEWS];
    u16        view_map[WR_RHI_MAX_VIEWS];

    u32        num_render_items;
    RenderItem render_items[WR_RHI_MAX_DRAW_CALLS + 1];
    RenderBind render_binds[WR_RHI_MAX_DRAW_CALLS + 1];

    u64 sortkeys[WR_RHI_MAX_DRAW_CALLS + 1];
    u32 sortvalues[WR_RHI_MAX_DRAW_CALLS + 1];

    Walrus_Resolution resolution;

    UniformBuffer *uniforms;

    mat4 matrix_cache[WR_RHI_MAX_MATRIX_CACHE];
    u32  num_matrices;

    u32 vbo_offset;
    u32 ibo_offset;
    u32 max_transient_vb;
    u32 max_transient_ib;

    Walrus_TransientBuffer *transient_vb;
    Walrus_TransientBuffer *transient_ib;

    u16 debug_flags;

    FREE_HANDLE(WR_RHI_MAX_BUFFERS, buffer);
    FREE_HANDLE(WR_RHI_MAX_VERTEX_LAYOUTS, layout);
    FREE_HANDLE(WR_RHI_MAX_TEXTURES, texture);
    FREE_HANDLE(WR_RHI_MAX_SHADERS, shader);
    FREE_HANDLE(WR_RHI_MAX_PROGRAMS, program);
    FREE_HANDLE(WR_RHI_MAX_UNIFORMS, uniform);
} RenderFrame;

void frame_init(RenderFrame *frame, u32 min_resource_cb, u32 max_transient_vb, u32 max_transient_ib);

void frame_shutdown(RenderFrame *frame);

void frame_reset(RenderFrame *frame);

void frame_reset_all_free_handles(RenderFrame *frame);

bool free_handle_queue_internal(Walrus_Handle *queue, u32 *num, Walrus_Handle x);

#define free_handle_queue(q, handle) free_handle_queue_internal(q.queue, &q.num, handle.id)

void frame_start(RenderFrame *frame);

void frame_finish(RenderFrame *frame);

void frame_sort(RenderFrame *frame);

u32 frame_add_matrices(RenderFrame *frame, mat4 const mat, u32 *num);

u32 frame_avail_transient_vb_size(RenderFrame *frame, u32 num, u16 stride, u16 align);

u32 frame_alloc_transient_vb(RenderFrame *frame, u32 *num, u16 stride, u16 align);

u32 frame_avail_transient_ib_size(RenderFrame *frame, u32 num, u16 stride, u16 align);

u32 frame_alloc_transient_ib(RenderFrame *frame, u32 *num, u16 stride, u16 align);

void draw_clear(RenderDraw *draw, u8 flags);

void compute_clear(RenderCompute *compute, u8 flags);

void bind_clear(RenderBind *bind, u8 flags);
