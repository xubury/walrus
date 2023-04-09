#pragma once

#include <rhi/rhi.h>

#include "frame.h"

typedef void (*Walrus_RhiSubmitFn)(Walrus_RenderFrame *frame);
typedef void (*Walrus_RhiCreateShaderFn)(Walrus_ShaderType type, Walrus_ShaderHandle handle, const char *source);
typedef void (*Walrus_RhiDestroyShaderFn)(Walrus_ShaderHandle handle);
typedef void (*Walrus_RhiCreateProgramFn)(Walrus_ProgramHandle handle, Walrus_ShaderHandle shader0,
                                          Walrus_ShaderHandle shader1, Walrus_ShaderHandle shader2);
typedef void (*Walrus_RhiDestroyProgramFn)(Walrus_ProgramHandle handle);

typedef struct {
    Walrus_RhiSubmitFn         submit_fn;
    Walrus_RhiCreateShaderFn   create_shader_fn;
    Walrus_RhiDestroyShaderFn  destroy_shader_fn;
    Walrus_RhiCreateProgramFn  create_program_fn;
    Walrus_RhiDestroyProgramFn destroy_program_fn;
} Walrus_RhiVTable;

typedef struct {
    Walrus_RhiFlag flags;

    Walrus_RenderDraw    draw;
    Walrus_RenderCompute compute;
    Walrus_RenderFrame   submit_frame;

    Walrus_RenderView views[WR_RHI_MAX_VIEWS];

    Walrus_HandleAlloc *shaders;
    Walrus_HandleAlloc *programs;

    Walrus_RhiError err;
    char const     *err_msg;
} Walrus_RhiContext;

#define WR_RHI_ALLOC_FAIL_STR    "Rhi fail to allocate memory"
#define WR_RHI_GL_ALLOC_FAIL_STR "Rhi's opengl context fail to allocate memory"

void init_gl_backend(Walrus_RhiContext *ctx, Walrus_RhiVTable *vtable);

void shutdown_gl_backend(void);
