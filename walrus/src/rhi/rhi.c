#include "rhi_p.h"
#include <core/macro.h>

#include <math.h>
#include <string.h>
#include <stdlib.h>

static char const* no_backend_str = "No render backend specifed";

static Walrus_RhiContext* s_ctx   = NULL;
static Walrus_RhiVTable*  s_table = NULL;

static void init_handles(void)
{
    s_ctx->shaders  = walrus_handle_create(WR_RHI_MAX_SHADERS);
    s_ctx->programs = walrus_handle_create(WR_RHI_MAX_PROGRAMS);
}

static void shutdown_handles(void)
{
    walrus_handle_destroy(s_ctx->shaders);
    walrus_handle_destroy(s_ctx->programs);
}

Walrus_RhiError walrus_rhi_init(Walrus_RhiFlag flags)
{
    s_ctx = malloc(sizeof(Walrus_RhiContext));
    if (s_ctx == NULL) {
        return WR_RHI_ALLOC_ERROR;
    }

    s_table = malloc(sizeof(Walrus_RhiVTable));
    if (s_table == NULL) {
        return WR_RHI_ALLOC_ERROR;
    }

    s_ctx->flags = flags;

    s_ctx->submit_frame.num_render_items = 0;
    s_ctx->submit_frame.num_views        = 0;

    if (s_ctx->flags & WR_RHI_FLAG_OPENGL) {
        init_gl_backend(s_ctx, s_table);
    }
    else {
        walrus_assert_msg(false, no_backend_str);
    }

    init_handles();

    return s_ctx->err;
}

void walrus_rhi_shutdown(void)
{
    shutdown_handles();
    shutdown_gl_backend();

    free(s_ctx);
    free(s_table);
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
}

void walrus_rhi_submit(i16 view_id, Walrus_ProgramHandle program)
{
    Walrus_RenderFrame* frame = &s_ctx->submit_frame;

    u32 const render_item_id = frame->num_render_items;
    frame->num_render_items   = fmin(WR_RHI_MAX_DRAW_CALLS, frame->num_render_items + 1);

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
