#include "frame.h"
#include <core/macro.h>
#include <core/math.h>

#include <cglm/mat4.h>
#include <string.h>

void frame_init(RenderFrame *frame, u32 max_transient_vb, u32 max_transient_ib)
{
    frame->num_render_items = 0;
    frame->uniforms         = uniform_buffer_create(1 << 20);
    frame->num_matrices     = 1;
    frame->vbo_offset       = 0;
    frame->max_transient_vb = max_transient_vb;
    frame->max_transient_ib = max_transient_ib;

    glm_mat4_copy((mat4)GLM_MAT4_IDENTITY_INIT, frame->matrix_cache[0]);
}

void frame_shutdown(RenderFrame *frame)
{
    uniform_buffer_destroy(frame->uniforms);
}

void frame_start(RenderFrame *frame)
{
    frame->num_render_items = 0;
    frame->num_matrices     = 1;
    frame->vbo_offset       = 0;
    frame->ibo_offset       = 0;
    uniform_buffer_start(frame->uniforms, 0);
}

void frame_finish(RenderFrame *frame)
{
    uniform_buffer_finish(frame->uniforms);
}

static u32 frame_reserve_matrices(RenderFrame *frame, u32 *pnum)
{
    u32 num             = *pnum;
    u32 first           = frame->num_matrices;
    frame->num_matrices = walrus_min(WR_RHI_MAX_MATRIX_CACHE - 1, walrus_u32satadd(num, frame->num_matrices));

    walrus_assert_msg(first + num < WR_RHI_MAX_MATRIX_CACHE, "Matrix cache overflow. {} (max:{})", first + num,
                      WR_RHI_MAX_MATRIX_CACHE);
    num   = walrus_min(num, WR_RHI_MAX_MATRIX_CACHE - 1 - first);
    *pnum = num;
    return first;
}

u32 frame_add_matrices(RenderFrame *frame, mat4 const mat, u32 *num)
{
    if (mat) {
        u32 first = frame_reserve_matrices(frame, num);
        memcpy(&frame->matrix_cache[first], mat, sizeof(mat4) * *num);
        return first;
    }
    return 0;
}
u32 frame_avail_transient_vb_size(RenderFrame *frame, u32 num, u16 stride)
{
    u32 const offset     = walrus_stride_align(frame->vbo_offset, stride);
    u32       vbo_offset = offset + num * stride;
    vbo_offset           = walrus_min(vbo_offset, frame->max_transient_vb);
    u32 const availNum   = (vbo_offset - offset) / stride;
    return availNum;
}

u32 frame_alloc_transient_vb(RenderFrame *frame, u32 *num, u16 stride)
{
    u32 offset        = walrus_stride_align(frame->vbo_offset, stride);
    *num              = frame_avail_transient_vb_size(frame, *num, stride);
    frame->vbo_offset = offset + *num * stride;
    return offset;
}

u32 frame_avail_transient_ib_size(RenderFrame *frame, u32 num, u16 stride)
{
    u32 const offset     = walrus_stride_align(frame->ibo_offset, stride);
    u32       ibo_offset = offset + num * stride;
    ibo_offset           = walrus_min(ibo_offset, frame->max_transient_ib);
    u32 const avail_num  = (ibo_offset - offset) / stride;
    return avail_num;
}

u32 frame_alloc_transient_ib(RenderFrame *frame, u32 *num, u16 stride)
{
    u32 offset        = walrus_stride_align(frame->ibo_offset, stride);
    *num              = frame_avail_transient_ib_size(frame, *num, stride);
    frame->ibo_offset = offset + *num * stride;
    return offset;
}

void draw_clear(RenderDraw *draw, u8 flags)
{
    if (flags & WR_RHI_DISCARD_STATE) {
        draw->uniform_begin = 0;
        draw->uniform_end   = 0;

        draw->state_flags  = WR_RHI_STATE_DEFAULT;
        draw->blend_factor = 0;
    }
    if (flags & WR_RHI_DISCARD_TRANSFORM) {
        draw->start_matrix = 0;
        draw->num_matrices = 0;
    }
    if (flags & WR_RHI_DISCARD_VERTEX_STREAMS) {
        draw->num_vertices                = UINT32_MAX;
        draw->stream_mask                 = 0;
        draw->streams[0].offset           = 0;
        draw->streams[0].handle.id        = WR_INVALID_HANDLE;
        draw->streams[0].layout_handle.id = WR_INVALID_HANDLE;
    }
    if (flags & WR_RHI_DISCARD_INDEX_BUFFER) {
        draw->num_indices     = UINT32_MAX;
        draw->index_buffer.id = WR_INVALID_HANDLE;
    }
    if (flags & WR_RHI_DISCARD_INSTANCE_DATA) {
        draw->instance_offset    = 0;
        draw->num_instances      = 1;
        draw->instance_buffer.id = WR_INVALID_HANDLE;
        draw->instance_layout.id = WR_INVALID_HANDLE;
    }
}

void bind_clear(RenderBind *bind, u8 flags)
{
    if (flags & WR_RHI_DISCARD_BINDINGS) {
        for (u32 i = 0; i < WR_RHI_MAX_TEXTURE_SAMPLERS; ++i) {
            Binding *binding       = &bind->bindings[i];
            binding->id            = WR_INVALID_HANDLE;
            binding->type          = WR_RHI_BIND_IMAGE;
            binding->sampler_flags = 0;
        }
    }
}
