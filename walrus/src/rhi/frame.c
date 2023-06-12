#include "frame.h"
#include "rhi_p.h"

#include <core/assert.h>
#include <core/math.h>
#include <core/sort.h>

#include <cglm/mat4.h>
#include <string.h>

#define SortKeyViewNumBits     (31 - walrus_u32cntlz(WR_RHI_MAX_VIEWS))
#define SortKeySortTypeNumBits (2)
#define SortKeyProgramNumBits  (32)
#define SortKeyBlendNumBits    (2)
#define SortKeyDepthNumBits    (16)
#define SortKeySeqNumBits      (16)

#define SortKeyViewBitShift (64 - SortKeyViewNumBits)
#define SortKeyViewMask     ((u64)((1 << SortKeyViewNumBits) - 1) << SortKeyViewBitShift)

// Bit determine draw or compute
#define SortKeyDrawBitShift (SortKeyViewBitShift - 1)
#define SortKeyDrawBit      ((u64)1 << SortKeyDrawBitShift)

// Draw key
// Bit determine sort type
#define SortKeySortTypeBitShift (SortKeyDrawBitShift - SortKeySortTypeNumBits)

#define SortKeySortTypeProgram  ((u64)0 << SortKeySortTypeBitShift)
#define SortKeySortTypeDepth    ((u64)1 << SortKeySortTypeBitShift)
#define SortKeySortTypeSequence ((u64)2 << SortKeySortTypeBitShift)
#define SorKeySortTypeMask      ((u64)3 << SortKeySortTypeBitShift)

// Sort by program
#define SortKeyBlend0BitShift (SortKeySortTypeBitShift - SortKeyBlendNumBits)
#define SortKeyBlend0Mask     ((((u64)1 << SortKeyBlendNumBits) - 1) << SortKeyBlend0BitShift)

#define SortKeyProgram0BitShift (SortKeyBlend0BitShift - SortKeyProgramNumBits)
#define SortKeyProgram0Mask     ((((u64)1 << SortKeyProgramNumBits) - 1) << SortKeyProgram0BitShift)

#define SortKeyDepth0BitShift (SortKeyProgram0BitShift - SortKeyDepthNumBits)
#define SortKeyDepth0Mask     ((((u64)1 << SortKeyDepthNumBits) - 1) << SortKeyDepth0BitShift)

// Sort by depth
#define SortKeyDepth1BitShift (SortKeySortTypeBitShift - SortKeyDepthNumBits)
#define SortKeyDepth1Mask     ((((u64)1 << SortKeyDepthNumBits) - 1) << SortKeyDepth1BitShift)

#define SortKeyBlend1BitShift (SortKeyDepth1BitShift - SortKeyBlendNumBits)
#define SortKeyBlend1Mask     ((((u64)1 << SortKeyBlendNumBits) - 1) << SortKeyBlend1BitShift)

#define SortKeyProgram1BitShift (SortKeyBlend1BitShift - SortKeyProgramNumBits)
#define SortKeyProgram1Mask     ((((u64)1 << SortKeyProgramNumBits) - 1) << SortKeyProgram1BitShift)

// Sort by sequence
#define SortKeySeq2BitShift (SortKeySortTypeBitShift - SortKeySeqNumBits)
#define SortKeySeq2Mask     ((((u64)1 << SortKeySeqNumBits) - 1) << SortKeySeq2BitShift)

#define SortKeyBlend2BitShift (SortKeySeq2BitShift - SortKeyBlendNumBits)
#define SortKeyBlend2Mask     ((((u64)1 << SortKeyBlendNumBits) - 1) << SortKeyBlend2BitShift)

#define SortKeyProgram2BitShift (SortKeyBlend2BitShift - SortKeyProgramNumBits)
#define SortKeyProgram2Mask     ((((u64)1 << SortKeyProgramNumBits) - 1) << SortKeyProgram2BitShift)

// Compute key
#define SortKeyComputeSeqBitShift (SortKeyDrawBitShift - SortKeySeqNumBits)
#define SortKeyComputeSeqBitMask  ((((u64)1 << SortKeySeqNumBits) - 1) << SortKeyComputeSeqBitShift)

#define SortKeyComputeProgramBitShift (SortKeyComputeSeqBitShift - SortKeyProgramNumBits)
#define SortKeyComputeProgramBitMask  ((((u64)1 << SortKeyProgramNumBits) - 1) << SortKeyComputeProgramBitShift)

u64 sortkey_encode_draw(Sortkey *key, SortKeyType type)
{
    switch (type) {
        case SORT_PROGRAM: {
            u64 const view    = ((u64)(key->view_id) << SortKeyViewBitShift) & SortKeyViewMask;
            u64 const blend   = ((u64)(key->blend) << SortKeyBlend0BitShift) & SortKeyBlend0Mask;
            u64 const program = ((u64)(key->program.id) << SortKeyProgram0BitShift) & SortKeyProgram0Mask;
            u64 const depth   = ((u64)(key->depth) << SortKeyDepth0BitShift) & SortKeyDepth0Mask;
            return view | SortKeyDrawBit | SortKeySortTypeProgram | blend | program | depth;
        } break;
        case SORT_DEPTH: {
            u64 const view    = ((u64)(key->view_id) << SortKeyViewBitShift) & SortKeyViewMask;
            u64 const blend   = ((u64)(key->blend) << SortKeyBlend1BitShift) & SortKeyBlend1Mask;
            u64 const depth   = ((u64)(key->depth) << SortKeyDepth1BitShift) & SortKeyDepth1Mask;
            u64 const program = ((u64)(key->program.id) << SortKeyProgram1BitShift) & SortKeyProgram1Mask;

            return view | SortKeyDrawBit | SortKeySortTypeDepth | depth | blend | program;
        } break;
        case SORT_SEQUENCE: {
            u64 const view    = ((u64)(key->view_id) << SortKeyViewBitShift) & SortKeyViewMask;
            u64 const seq     = ((u64)(key->sequence) << SortKeySeq2BitShift) & SortKeySeq2Mask;
            u64 const blend   = ((u64)(key->blend) << SortKeyBlend2BitShift) & SortKeyBlend2Mask;
            u64 const program = ((u64)(key->program.id) << SortKeyProgram2BitShift) & SortKeyProgram2Mask;
            return view | SortKeyDrawBit | SortKeySortTypeSequence | seq | blend | program;
        } break;
    }
    return 0;
}

u64 sortkey_encode_compute(Sortkey *key)
{
    u64 const program = ((u64)(key->program.id) << SortKeyComputeProgramBitShift) & SortKeyComputeProgramBitMask;
    u64 const seq     = ((u64)(key->sequence) << SortKeyComputeSeqBitShift) & SortKeyComputeSeqBitMask;
    u64 const view    = ((u64)(key->view_id) << SortKeyViewBitShift) & SortKeyViewMask;
    u64 const key_val = program | seq | view;

    walrus_assert_msg(seq == ((u64)(key->sequence) << SortKeyComputeSeqBitShift),
                      "SortKey error, sequence is truncated (Sequence: %d).", key->sequence);
    return key_val;
}

void sortkey_reset(Sortkey *key)
{
    key->depth      = 0;
    key->blend      = 0;
    key->view_id    = 0;
    key->program.id = 0;
    key->sequence   = 0;
}

bool sortkey_decode(Sortkey *key, u64 key_val, u16 *view_map)
{
    key->view_id = view_map[sortkey_decode_view(key_val)];

    if (key_val & SortKeyDrawBit) {
        u64 type = key_val & SorKeySortTypeMask;

        if (type == SortKeySortTypeProgram) {
            key->program.id = (key_val & SortKeyProgram0Mask) >> SortKeyProgram0BitShift;
        }
        else if (type == SortKeySortTypeDepth) {
            key->program.id = (key_val & SortKeyProgram1Mask) >> SortKeyProgram1BitShift;
        }
        else if (type == SortKeySortTypeSequence) {
            key->program.id = (key_val & SortKeyProgram2Mask) >> SortKeyProgram2BitShift;
        }
        else {
            walrus_assert_msg(false, "SortKey error, illegal key!");
        }
        return false;
    }
    else {
        key->program.id = (key_val & SortKeyComputeProgramBitMask) >> SortKeyComputeProgramBitShift;
        return true;
    }
}

u16 sortkey_decode_view(u64 key_val)
{
    return (u16)((key_val & SortKeyViewMask) >> SortKeyViewBitShift);
}

u64 sortkey_remapview(u64 key_val, u16 *view_map)
{
    u16 const old_view = sortkey_decode_view(key_val);
    u64 const view     = (u64)(view_map[old_view]) << SortKeyViewBitShift;
    return (key_val & ~SortKeyViewMask) | view;
}

void frame_init(RenderFrame *frame, u32 min_resource_cb, u32 max_transient_vb, u32 max_transient_ib)
{
    Sortkey term;
    sortkey_reset(&term);
    term.program.id                          = WR_INVALID_HANDLE;
    frame->sortkeys[WR_RHI_MAX_DRAW_CALLS]   = sortkey_encode_draw(&term, SORT_PROGRAM);
    frame->sortvalues[WR_RHI_MAX_DRAW_CALLS] = WR_RHI_MAX_DRAW_CALLS;

    frame->num_render_items = 0;
    frame->uniforms         = uniform_buffer_create(1 << 20);
    frame->num_matrices     = 1;
    frame->vbo_offset       = 0;
    frame->max_transient_vb = max_transient_vb;
    frame->max_transient_ib = max_transient_ib;

    glm_mat4_copy((mat4)GLM_MAT4_IDENTITY_INIT, frame->matrix_cache[0]);

    command_buffer_init(&frame->cmd_pre, min_resource_cb);
    command_buffer_init(&frame->cmd_post, min_resource_cb);

    frame_reset(frame);
    frame_start(frame);
}

void frame_shutdown(RenderFrame *frame)
{
    uniform_buffer_destroy(frame->uniforms);
}

void frame_reset(RenderFrame *frame)
{
    frame_start(frame);
    frame_finish(frame);
}

bool free_handle_queue_internal(Walrus_Handle *queue, u32 *num, Walrus_Handle x)
{
    for (u32 i = 0; i < *num; ++i) {
        if (queue[i] == x) {
            return false;
        }
    }
    queue[*num] = x;
    ++*num;
    return true;
}

WR_INLINE void free_handle_reset(u32 *num)
{
    *num = 0;
}

void frame_reset_all_free_handles(RenderFrame *frame)
{
    free_handle_reset(&frame->queue_buffer.num);
    free_handle_reset(&frame->queue_layout.num);
    free_handle_reset(&frame->queue_texture.num);
    free_handle_reset(&frame->queue_shader.num);
    free_handle_reset(&frame->queue_program.num);
    free_handle_reset(&frame->queue_uniform.num);
}

void frame_start(RenderFrame *frame)
{
    frame->num_render_items = 0;
    frame->num_matrices     = 1;
    frame->vbo_offset       = 0;
    frame->ibo_offset       = 0;
    command_buffer_start(&frame->cmd_pre);
    command_buffer_start(&frame->cmd_post);
    uniform_buffer_start(frame->uniforms, 0);
}

void frame_finish(RenderFrame *frame)
{
    command_buffer_finish(&frame->cmd_pre);
    command_buffer_finish(&frame->cmd_post);
    uniform_buffer_finish(frame->uniforms);
}

void frame_sort(RenderFrame *frame)
{
    u16 view_remap[WR_RHI_MAX_VIEWS];
    for (u16 i = 0; i < WR_RHI_MAX_VIEWS; ++i) {
        view_remap[frame->view_map[i]] = i;

        ViewRect rect = {0, 0, frame->resolution.width, frame->resolution.height};

        viewrect_intersect(&frame->views[i].viewport, &rect);
        if (!viewrect_zero_area(&frame->views[i].scissor)) {
            viewrect_intersect(&frame->views[i].scissor, &rect);
        }
    }
    for (u32 i = 0; i < frame->num_render_items; ++i) {
        frame->sortkeys[i] = sortkey_remapview(frame->sortkeys[i], view_remap);
    }
    static u64 tmp_keys[WR_RHI_MAX_DRAW_CALLS];
    static u32 tmp_values[WR_RHI_MAX_DRAW_CALLS];
    walrus_radix_sort64(frame->sortkeys, tmp_keys, frame->sortvalues, tmp_values, frame->num_render_items,
                        sizeof(tmp_values[0]));
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
        draw->stencil      = pack_stencil(WR_RHI_STENCIL_DEFAULT, WR_RHI_STENCIL_DEFAULT);
        draw->blend_factor = 0;
        draw->scissor      = (ViewRect){0, 0, 0, 0};
    }
    if (flags & WR_RHI_DISCARD_TRANSFORM) {
        draw->start_matrix = 0;
        draw->num_matrices = 1;
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
        draw->index_size      = 0;
        draw->index_offset    = 0;
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
        for (u32 i = 0; i < WR_RHI_MAX_UNIFORM_BINDINGS; ++i) {
            BlockBinding *binding = &bind->block_bindings[i];
            binding->handle.id    = WR_INVALID_HANDLE;
            binding->offset       = 0;
            binding->size         = 0;
        }
    }
}
