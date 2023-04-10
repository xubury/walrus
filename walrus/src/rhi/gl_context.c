#include "rhi_p.h"
#include "gl_shader.h"

#include <core/macro.h>
#include <core/log.h>

#include <stdlib.h>

GLenum glew_init(void);

void wajs_setup_gl_context(void);

Walrus_GlContext *g_ctx = NULL;

static void submit(Walrus_RenderFrame *frame)
{
    u32 const resolution_height = frame->resolution.height;

    glBindVertexArray(g_ctx->vao);

    u16 view_id = UINT16_MAX;

    Walrus_ProgramHandle current_prog = {WR_INVALID_HANDLE};

    glEnable(GL_DEPTH_TEST);

    for (i32 i = 0; i < frame->num_render_items; ++i) {
        Walrus_RenderItem const    *render_item  = &frame->render_items[i];
        Walrus_RenderDraw const    *draw         = &render_item->draw;
        Walrus_RenderCompute const *compute      = &render_item->compute;
        Walrus_ProgramHandle const  prog         = frame->program[i];
        bool const                  view_changed = frame->view_ids[i] != view_id;

        if (view_changed) {
            view_id = frame->view_ids[i];

            Walrus_RenderView const  *view     = &frame->views[view_id];
            Walrus_RenderClear const *clear    = &view->clear;
            Walrus_Rect const        *viewport = &view->viewport;

            glViewport(viewport->x, resolution_height - viewport->height - viewport->y, viewport->width,
                       viewport->height);

            GLbitfield clear_flags = 0;
            if (WR_RHI_CLEAR_COLOR & clear->flags) {
                glClearColor(clear->index[0] / 255.f, clear->index[1] / 255.f, clear->index[2] / 255.f,
                             clear->index[3] / 255.f);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                clear_flags |= GL_COLOR_BUFFER_BIT;
            }
            if (WR_RHI_CLEAR_DEPTH & clear->flags) {
                glClearDepth(clear->depth);
                glDepthMask(GL_TRUE);
                clear_flags |= GL_DEPTH_BUFFER_BIT;
            }
            if (WR_RHI_CLEAR_STENCIL & clear->flags) {
                glClearStencil(clear->stencil);
                clear_flags |= GL_STENCIL_BUFFER_BIT;
            }
            if (clear_flags != 0) {
                glClear(clear_flags);
            }
        }

        bool const programChanged = prog.id != WR_INVALID_HANDLE && current_prog.id != prog.id;
        if (programChanged) {
            current_prog = prog;
            glUseProgram(g_ctx->programs[current_prog.id]);
        }

        glDrawArrays(GL_TRIANGLES, 0, 36);

        walrus_unused(draw);
        walrus_unused(compute);
    }
}

static void init_ctx(Walrus_RhiContext *rhi)
{
#if WR_PLATFORM != WR_PLATFORM_WASM
    GLenum err = glew_init();
    if (err != GLEW_OK) {
        rhi->err     = WR_RHI_INIT_GLEW_ERROR;
        rhi->err_msg = (char const *)glewGetErrorString(err);

        return;
    }
#else
    wajs_setup_gl_context();
#endif
    g_ctx = malloc(sizeof(Walrus_GlContext));
    if (g_ctx == NULL) {
        rhi->err     = WR_RHI_ALLOC_ERROR;
        rhi->err_msg = WR_RHI_GL_ALLOC_FAIL_STR;
        return;
    }

    glGenVertexArrays(1, &g_ctx->vao);
    glBindVertexArray(g_ctx->vao);
}

static void init_api(Walrus_RhiVTable *vtable)
{
    vtable->submit_fn          = submit;
    vtable->create_shader_fn   = gl_create_shader;
    vtable->destroy_shader_fn  = gl_destroy_shader;
    vtable->create_program_fn  = gl_create_program;
    vtable->destroy_program_fn = gl_destroy_program;
}

void init_gl_backend(Walrus_RhiContext *ctx, Walrus_RhiVTable *vtable)
{
    if (ctx) {
        init_ctx(ctx);
    }

    if (vtable) {
        init_api(vtable);
    }
}

void shutdown_gl_backend(void)
{
    if (g_ctx) {
        glDeleteVertexArrays(1, &g_ctx->vao);
        free(g_ctx);
        g_ctx = NULL;
    }
}
