#include "frame.h"
#include <core/macro.h>
#include <core/math.h>

#include <cglm/mat4.h>
#include <string.h>

void frame_init(RenderFrame *frame)
{
    frame->num_render_items = 0;
    frame->uniforms         = uniform_buffer_create(1 << 20);
    frame->num_matrices     = 1;

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

void draw_clear(RenderDraw *draw, u8 flags)
{
    if (flags & WR_RHI_DISCARD_STATE) {
        draw->uniform_begin = 0;
        draw->uniform_end   = 0;
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
}
