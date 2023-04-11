#include "rhi_p.h"
#include "gl_shader.h"
#include "frame.h"

#include <core/macro.h>
#include <core/log.h>
#include <core/memory.h>
#include <core/string.h>

#include <string.h>
#include <cglm/cglm.h>
#include <cglm/io.h>

GLenum glew_init(void);

void wajs_setup_gl_context(void);

GlContext *g_ctx = NULL;

static void commit(GlProgram *program)
{
    UniformBuffer *buffer = program->buffer;
    if (buffer == NULL) {
        return;
    }

    uniform_buffer_start(buffer, 0);

    while (true) {
        u64 op = uniform_buffer_read_value(buffer);
        if (op == UNIFORM_BUFFER_END) {
            break;
        }

        Walrus_UniformHandle handle;
        memcpy(&handle, uniform_buffer_read(buffer, sizeof(Walrus_UniformHandle)), sizeof(Walrus_UniformHandle));
        void *data = g_ctx->uniforms[handle.id];

        Walrus_UniformType type;
        u32                loc;
        u8                 num;
        uniform_decode_op(&type, &loc, &num, op);
        switch (type) {
            case WR_RHI_UNIFORM_BOOL:
            case WR_RHI_UNIFORM_UINT: {
                if (num > 1) {
                    glUniform1uiv(loc, num, (uint32_t *)data);
                }
                else {
                    glUniform1ui(loc, *(uint32_t *)data);
                }
            } break;
            case WR_RHI_UNIFORM_INT: {
                if (num > 1) {
                    glUniform1iv(loc, num, (int32_t *)data);
                }
                else {
                    glUniform1i(loc, *(int32_t *)data);
                }
            } break;
            case WR_RHI_UNIFORM_FLOAT: {
                glUniform1fv(loc, num, (float *)data);
            } break;
            case WR_RHI_UNIFORM_VEC2: {
                glUniform2fv(loc, num, (const GLfloat *)data);
            } break;
            case WR_RHI_UNIFORM_VEC3: {
                glUniform3fv(loc, num, (const GLfloat *)data);
            } break;
            case WR_RHI_UNIFORM_VEC4: {
                glUniform4fv(loc, num, (const GLfloat *)data);
            } break;
            case WR_RHI_UNIFORM_MAT3: {
                glUniformMatrix3fv(loc, num, GL_FALSE, (const GLfloat *)data);
            } break;
            case WR_RHI_UNIFORM_MAT4: {
                glUniformMatrix4fv(loc, num, GL_FALSE, (const GLfloat *)data);
            } break;
            case WR_RHI_UNIFORM_SAMPLER: {
                if (num > 1) {
                    glUniform1iv(loc, num, (int32_t *)data);
                }
                else {
                    glUniform1i(loc, *(int32_t *)data);
                }
            } break;
            case WR_RHI_UNIFORM_COUNT:
                break;
        }
    }
}

static void set_predefineds(GlProgram *prog, RenderFrame const *frame, RenderView const *view, u32 start_matrix,
                            u32 num_matrices)
{
    walrus_unused(start_matrix);
    walrus_unused(num_matrices);
    walrus_unused(frame);
    for (u8 i = 0; i < prog->num_predefineds; ++i) {
        PredefinedUniform *uni = &prog->predefineds[i];
        switch (uni->type) {
            case PREDEFINED_VIEW:
                glUniformMatrix4fv(uni->loc, 1, GL_FALSE, &view->view[0][0]);
                break;
            case PREDEFINED_VIEWPROJ: {
                mat4 viewproj;
                glm_mat4_mul((vec4 *)view->view, (vec4 *)view->projection, viewproj);
                glUniformMatrix4fv(uni->loc, 1, GL_FALSE, &viewproj[0][0]);
            } break;
            case PREDEFINED_MODEL:
                glUniformMatrix4fv(uni->loc, num_matrices, GL_FALSE, &frame->matrix_cache[start_matrix][0][0]);
                break;
            case PREDEFINED_COUNT:
                break;
        }
    }
}

static void submit(RenderFrame *frame)
{
    u32 const resolution_height = frame->resolution.height;

    glBindVertexArray(g_ctx->vao);

    u16 view_id = UINT16_MAX;

    Walrus_ProgramHandle current_prog = {WR_INVALID_HANDLE};

    glEnable(GL_DEPTH_TEST);

    for (u32 i = 0; i < frame->num_render_items; ++i) {
        RenderItem const          *render_item = &frame->render_items[i];
        RenderDraw const          *draw        = &render_item->draw;
        RenderCompute const       *compute     = &render_item->compute;
        Walrus_ProgramHandle const prog        = frame->program[i];
        RenderView const          *view        = &frame->views[frame->view_ids[i]];

        bool const view_changed = frame->view_ids[i] != view_id;

        if (view_changed) {
            view_id = frame->view_ids[i];

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
            glUseProgram(g_ctx->programs[current_prog.id].id);
        }

        bool const constants_changed = draw->uniform_begin < draw->uniform_end;
        if (current_prog.id != WR_INVALID_HANDLE) {
            GlProgram *program = &g_ctx->programs[current_prog.id];
            if (programChanged || constants_changed) {
                commit(program);
            }
            set_predefineds(program, frame, view, draw->start_matrix, draw->num_matrices);
        }

        walrus_unused(compute);

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

static void init_ctx(RhiContext *rhi)
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
    g_ctx = walrus_malloc(sizeof(GlContext));
    if (g_ctx == NULL) {
        rhi->err     = WR_RHI_ALLOC_ERROR;
        rhi->err_msg = WR_RHI_GL_ALLOC_FAIL_STR;
        return;
    }

    g_ctx->uniform_registry = walrus_hash_table_create(walrus_str_hash, walrus_str_equal);

    glGenVertexArrays(1, &g_ctx->vao);
    glBindVertexArray(g_ctx->vao);
}

static void gl_create_uniform(Walrus_UniformHandle handle, const char *name, i32 size)
{
    g_ctx->uniforms[handle.id]      = walrus_malloc0(size);
    g_ctx->uniform_names[handle.id] = walrus_str_dup(name);
    walrus_hash_table_insert(g_ctx->uniform_registry, g_ctx->uniform_names[handle.id], walrus_u32_to_ptr(handle.id));
}

static void gl_destroy_uniform(Walrus_UniformHandle handle)
{
    walrus_hash_table_remove(g_ctx->uniform_registry, g_ctx->uniform_names[handle.id]);
    walrus_free(g_ctx->uniforms[handle.id]);
    walrus_str_free(g_ctx->uniform_names[handle.id]);

    g_ctx->uniforms[handle.id]      = NULL;
    g_ctx->uniform_names[handle.id] = NULL;
}

static void gl_update_uniform(Walrus_UniformHandle handle, u32 offset, u32 size, void const *data)
{
    memcpy((u8 *)&g_ctx->uniforms[handle.id] + offset, data, size);
}

static void init_api(RhiVTable *vtable)
{
    vtable->submit_fn = submit;

    vtable->create_shader_fn  = gl_shader_create;
    vtable->destroy_shader_fn = gl_shader_destroy;

    vtable->create_program_fn  = gl_program_create;
    vtable->destroy_program_fn = gl_program_destroy;

    vtable->create_uniform_fn  = gl_create_uniform;
    vtable->destroy_uniform_fn = gl_destroy_uniform;
    vtable->update_uniform_fn  = gl_update_uniform;
}

void gl_backend_init(RhiContext *rhi, RhiVTable *vtable)
{
    init_ctx(rhi);
    init_api(vtable);
}

void gl_backend_shutdown(void)
{
    if (g_ctx) {
        walrus_hash_table_destroy(g_ctx->uniform_registry);
        glDeleteVertexArrays(1, &g_ctx->vao);
        walrus_free(g_ctx);
        g_ctx = NULL;
    }
}
