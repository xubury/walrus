#include "rhi_p.h"

#include <core/macro.h>
#include <core/log.h>

#include <stdlib.h>

GLenum glew_init(void);

void wajs_setup_gl_context(void);

typedef struct {
    GLuint vao;
    GLuint shaders[WR_RHI_MAX_SHADERS];
    GLuint programs[WR_RHI_MAX_PROGRAMS];
} Walrus_GlContext;

static Walrus_GlContext *s_gl = NULL;

static void submit(Walrus_RenderFrame *frame)
{
    u32 const resolution_height = frame->resolution.height;

    glBindVertexArray(s_gl->vao);

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
            glUseProgram(s_gl->programs[current_prog.id]);
        }

        glDrawArrays(GL_TRIANGLES, 0, 36);

        walrus_unused(draw);
        walrus_unused(compute);
    }
}

static void init_ctx(Walrus_RhiContext *ctx)
{
#if WR_PLATFORM != WR_PLATFORM_WASM
    GLenum err = glew_init();
    if (err != GLEW_OK) {
        ctx->err     = WR_RHI_INIT_GLEW_ERROR;
        ctx->err_msg = (char const *)glewGetErrorString(err);

        return;
    }
#else
    wajs_setup_gl_context();
#endif
    s_gl = malloc(sizeof(Walrus_GlContext));
    if (s_gl == NULL) {
        ctx->err     = WR_RHI_ALLOC_ERROR;
        ctx->err_msg = WR_RHI_GL_ALLOC_FAIL_STR;
        return;
    }

    glGenVertexArrays(1, &s_gl->vao);
    glBindVertexArray(s_gl->vao);
}

static GLenum const s_shader_type[] = {GL_COMPUTE_SHADER, GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};

static void create_shader(Walrus_ShaderType type, Walrus_ShaderHandle handle, const char *source)
{
    GLenum shader = glCreateShader(s_shader_type[type]);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint succ;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &succ);
    if (!succ) {
        GLint logSize;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);

        char *infoLog    = (char *)malloc(logSize);
        infoLog[logSize] = 0;
        glGetShaderInfoLog(shader, logSize, NULL, infoLog);
        walrus_error("Shader compile error: %s", infoLog);
        free(infoLog);
    }
    s_gl->shaders[handle.id] = shader;
}

static void destroy_shader(Walrus_ShaderHandle handle)
{
    glDeleteShader(s_gl->shaders[handle.id]);
    s_gl->shaders[handle.id] = 0;
}

static void create_program(Walrus_ProgramHandle handle, Walrus_ShaderHandle shader0, Walrus_ShaderHandle shader1,
                           Walrus_ShaderHandle shader2)
{
    GLuint prog = glCreateProgram();
    if (shader0.id != WR_INVALID_HANDLE) {
        glAttachShader(prog, s_gl->shaders[shader0.id]);
    }
    if (shader1.id != WR_INVALID_HANDLE) {
        glAttachShader(prog, s_gl->shaders[shader1.id]);
    }
    if (shader2.id != WR_INVALID_HANDLE) {
        glAttachShader(prog, s_gl->shaders[shader2.id]);
    }
    glLinkProgram(prog);

    GLint succ;
    glGetProgramiv(prog, GL_LINK_STATUS, &succ);
    if (!succ) {
        GLint logSize = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logSize);

        char *infoLog    = (char *)malloc(logSize);
        infoLog[logSize] = 0;
        glGetProgramInfoLog(prog, logSize, NULL, infoLog);
        walrus_error("Failed to link shader program: %s", infoLog);
        free(infoLog);
    }

    s_gl->programs[handle.id] = prog;
}

void destroy_program(Walrus_ProgramHandle handle)
{
    glDeleteProgram(s_gl->programs[handle.id]);
}

static void init_api(Walrus_RhiVTable *vtable)
{
    vtable->submit_fn          = submit;
    vtable->create_shader_fn   = create_shader;
    vtable->destroy_shader_fn  = destroy_shader;
    vtable->create_program_fn  = create_program;
    vtable->destroy_program_fn = destroy_program;
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
    if (s_gl) {
        glDeleteVertexArrays(1, &s_gl->vao);
        free(s_gl);
        s_gl = NULL;
    }
}
