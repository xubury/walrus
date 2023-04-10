#include "rhi_p.h"
#include <core/macro.h>
#include <core/memory.h>
#include <core/string.h>

#include <math.h>
#include <string.h>

static char const* no_backend_str = "No render backend specifed";

static Walrus_RhiContext* s_ctx   = NULL;
static Walrus_RhiVTable*  s_table = NULL;

static void init_handles(void)
{
    s_ctx->shaders  = walrus_handle_create(WR_RHI_MAX_SHADERS);
    s_ctx->programs = walrus_handle_create(WR_RHI_MAX_PROGRAMS);
    s_ctx->uniforms = walrus_handle_create(WR_RHI_MAX_UNIFORMS);
}

static void shutdown_handles(void)
{
    walrus_handle_destroy(s_ctx->uniforms);
    walrus_handle_destroy(s_ctx->shaders);
    walrus_handle_destroy(s_ctx->programs);
}

void renderer_update_uniforms(UniformBuffer* uniform, u32 begin, u32 end)
{
    uniform_buffer_start(uniform, begin);
    while (uniform->pos < end) {
        u64 op = uniform_buffer_read_value(uniform);
        if (op == UNIFORM_BUFFER_END) {
            break;
        }

        Walrus_UniformType   type;
        Walrus_UniformHandle handle;

        uniform_decode_op(&type, &handle.id, NULL, op);
        u32         offset = uniform_buffer_read_value(uniform);
        u32         size   = uniform_buffer_read_value(uniform);
        void const* data   = uniform_buffer_read(uniform, size);
        if (type < WR_RHI_UNIFORM_COUNT) {
            s_table->update_uniform_fn(handle, offset, size, data);
        }
    }
}

Walrus_RhiError walrus_rhi_init(Walrus_RhiFlag flags)
{
    s_ctx = walrus_malloc(sizeof(Walrus_RhiContext));
    if (s_ctx == NULL) {
        return WR_RHI_ALLOC_ERROR;
    }

    s_table = walrus_malloc(sizeof(Walrus_RhiVTable));
    if (s_table == NULL) {
        return WR_RHI_ALLOC_ERROR;
    }

    s_ctx->flags = flags;

    s_ctx->submit_frame.num_render_items = 0;
    s_ctx->submit_frame.num_views        = 0;

    s_ctx->submit_frame.uniforms = uniform_buffer_create(1 << 20);
    s_ctx->uniform_begin         = 0;
    s_ctx->uniform_end           = 0;

    if (s_ctx->flags & WR_RHI_FLAG_OPENGL) {
        init_gl_backend(s_ctx, s_table);
    }
    else {
        walrus_assert_msg(false, no_backend_str);
    }

    init_handles();

    s_ctx->uniform_map = walrus_hash_table_create(walrus_str_hash, walrus_str_equal);
    for (u32 i = 0; i < WR_RHI_MAX_UNIFORMS; ++i) {
        s_ctx->uniform_refs[i].ref_count = 0;
    }

    return s_ctx->err;
}

void walrus_rhi_shutdown(void)
{
    walrus_hash_table_destroy(s_ctx->uniform_map);

    shutdown_handles();
    shutdown_gl_backend();

    walrus_free(s_ctx);
    walrus_free(s_table);
    s_ctx   = NULL;
    s_table = NULL;
}

char const* walrus_rhi_error_msg(void)
{
    if (s_ctx == NULL) {
        return WR_RHI_ALLOC_FAIL_STR;
    }
    else {
        return s_ctx->err_msg;
    }
}

void walrus_rhi_set_resolution(i32 width, i32 height)
{
    s_ctx->submit_frame.resolution.width  = width;
    s_ctx->submit_frame.resolution.height = height;
}

void walrus_rhi_frame(void)
{
    Walrus_RenderFrame* frame = &s_ctx->submit_frame;
    memcpy(frame->views, s_ctx->views, sizeof(s_ctx->views));

    s_table->submit_fn(frame);

    s_ctx->uniform_begin = 0;
    s_ctx->uniform_end   = 0;

    frame->num_render_items = 0;
    uniform_buffer_start(frame->uniforms, 0);
}

void walrus_rhi_submit(i16 view_id, Walrus_ProgramHandle program)
{
    Walrus_RenderFrame* frame = &s_ctx->submit_frame;

    u32 const render_item_id = frame->num_render_items;
    frame->num_render_items  = fmin(WR_RHI_MAX_DRAW_CALLS, frame->num_render_items + 1);

    s_ctx->uniform_end        = frame->uniforms->pos;
    s_ctx->draw.uniform_begin = s_ctx->uniform_begin;
    s_ctx->draw.uniform_end   = s_ctx->uniform_end;

    frame->program[render_item_id]           = program;
    frame->view_ids[render_item_id]          = view_id;
    frame->render_items[render_item_id].draw = s_ctx->draw;
}

u32 walrus_rhi_compose_rgba(u8 r, u8 g, u8 b, u8 a)
{
    return (u32)(r) | (u32)(g) << 8 | (u32)(b) << 16 | (u32)(a) << 24;
}

void walrus_rhi_decompose_rgba(u32 rgba, u8* r, u8* g, u8* b, u8* a)
{
    *r = (u8)(rgba >> 0);
    *g = (u8)(rgba >> 8);
    *b = (u8)(rgba >> 16);
    *a = (u8)(rgba >> 24);
}

void walrus_rhi_set_view_rect(i16 view_id, i32 x, i32 y, i32 width, i32 height)
{
    width                   = fmax(width, 1);
    height                  = fmax(height, 1);
    Walrus_RenderView* view = &s_ctx->views[view_id];

    view->viewport.x      = x;
    view->viewport.y      = y;
    view->viewport.width  = width;
    view->viewport.height = height;
}

void walrus_rhi_set_view_clear(i16 view_id, u16 flags, u32 rgba, f32 depth, u8 stencil)
{
    Walrus_RenderClear* clear = &s_ctx->views[view_id].clear;

    walrus_rhi_decompose_rgba(rgba, &clear->index[0], &clear->index[1], &clear->index[2], &clear->index[3]);
    clear->flags   = flags;
    clear->depth   = depth;
    clear->stencil = stencil;
}

Walrus_ShaderHandle walrus_rhi_create_shader(Walrus_ShaderType type, char const* source)
{
    Walrus_ShaderHandle handle = {walrus_handle_alloc(s_ctx->shaders)};
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return handle;
    }

    s_table->create_shader_fn(type, handle, source);

    return handle;
}

void walrus_rhi_destroy_shader(Walrus_ShaderHandle handle)
{
    s_table->destroy_shader_fn(handle);

    walrus_handle_free(s_ctx->shaders, handle.id);
}

Walrus_ProgramHandle walrus_rhi_create_program(Walrus_ShaderHandle vs, Walrus_ShaderHandle fs)
{
    Walrus_ProgramHandle handle = {walrus_handle_alloc(s_ctx->programs)};
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return handle;
    }

    s_table->create_program_fn(handle, vs, fs, (Walrus_ShaderHandle){WR_INVALID_HANDLE});

    return handle;
}
void walrus_rhi_destroy_program(Walrus_ProgramHandle handle)
{
    s_table->destroy_program_fn(handle);
    walrus_handle_free(s_ctx->programs, handle.id);
}

static uint16_t get_uniform_size(Walrus_UniformType type)
{
    switch (type) {
        case WR_RHI_UNIFORM_SAMPLER:
        case WR_RHI_UNIFORM_INT:
        case WR_RHI_UNIFORM_UINT:
        case WR_RHI_UNIFORM_BOOL:
            return sizeof(int32_t);
        case WR_RHI_UNIFORM_FLOAT:
            return sizeof(float);
        case WR_RHI_UNIFORM_VEC2:
            return 2 * sizeof(float);
        case WR_RHI_UNIFORM_VEC3:
            return 3 * sizeof(float);
        case WR_RHI_UNIFORM_VEC4:
            return 4 * sizeof(float);
        case WR_RHI_UNIFORM_MAT3:
            return 3 * 3 * sizeof(float);
        case WR_RHI_UNIFORM_MAT4:
            return 4 * 4 * sizeof(float);
        case WR_RHI_UNIFORM_COUNT:
            return 0;
    }
    return 0;
}

Walrus_UniformHandle walrus_rhi_create_uniform(char const* name, Walrus_UniformType type, i8 num)
{
    Walrus_UniformHandle handle = {WR_INVALID_HANDLE};

    u32 const size = num * get_uniform_size(type);
    if (walrus_hash_table_contains(s_ctx->uniform_map, name)) {
        handle.id              = walrus_ptr_to_u32(walrus_hash_table_lookup(s_ctx->uniform_map, name));
        Walrus_UniformRef* ref = &s_ctx->uniform_refs[handle.id];
        if (ref->size < size) {
            // resize uniform
        }

        ++ref->ref_count;
    }
    else {
        handle = (Walrus_UniformHandle){walrus_handle_alloc(s_ctx->uniforms)};
        if (handle.id == WR_INVALID_HANDLE) {
            s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
            return handle;
        }

        Walrus_UniformRef* ref = &s_ctx->uniform_refs[handle.id];
        ref->name              = walrus_str_dup(name);
        ref->type              = type;
        ref->size              = size;
        ref->ref_count         = 1;
        walrus_hash_table_insert(s_ctx->uniform_map, ref->name, walrus_u32_to_ptr(handle.id));

        s_table->create_uniform_fn(handle, name, size);
    }

    return handle;
}

void walrus_rhi_destroy_uniform(Walrus_UniformHandle handle)
{
    Walrus_UniformRef* ref = &s_ctx->uniform_refs[handle.id];
    if (ref->ref_count > 0) {
        --ref->ref_count;
        if (ref->ref_count == 0) {
            walrus_assert(walrus_hash_table_remove(s_ctx->uniform_map, ref->name));

            walrus_str_free(ref->name);
            walrus_handle_free(s_ctx->uniforms, handle.id);

            s_table->destroy_uniform_fn(handle);
        }
    }
}

void walrus_rhi_set_uniform(Walrus_UniformHandle handle, u32 offset, u32 size, void const* data)
{
    Walrus_UniformRef* ref = &s_ctx->uniform_refs[handle.id];
    if (ref->ref_count > 0) {
        uniform_buffer_update(&s_ctx->submit_frame.uniforms, 64 << 10, 1 << 20);
        uniform_buffer_write_uniform(s_ctx->submit_frame.uniforms, ref->type, handle, offset, size, data);
    }
    else {
        walrus_error("Cannot find valid uniform!");
    }
}
