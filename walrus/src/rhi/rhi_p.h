#pragma once

#include <rhi/rhi.h>
#include <core/hash.h>

#include "frame.h"
#include "uniform_buffer.h"

typedef void (*Walrus_RhiSubmitFn)(Walrus_RenderFrame *frame);
typedef void (*Walrus_RhiCreateShaderFn)(Walrus_ShaderType type, Walrus_ShaderHandle handle, char const *source);
typedef void (*Walrus_RhiDestroyShaderFn)(Walrus_ShaderHandle handle);
typedef void (*Walrus_RhiCreateProgramFn)(Walrus_ProgramHandle handle, Walrus_ShaderHandle shader0,
                                          Walrus_ShaderHandle shader1, Walrus_ShaderHandle shader2);
typedef void (*Walrus_RhiDestroyProgramFn)(Walrus_ProgramHandle handle);
typedef void (*Walrus_RhiCreateUniformFn)(Walrus_UniformHandle handle, const char *name, i32 size);
typedef void (*Walrus_RhiDestroyUniformFn)(Walrus_UniformHandle handle);
typedef void (*Walrus_RhiUpdateUniformFn)(Walrus_UniformHandle handle, u32 offset, u32 size, void const *data);

typedef struct {
    Walrus_RhiSubmitFn submit_fn;

    Walrus_RhiCreateShaderFn  create_shader_fn;
    Walrus_RhiDestroyShaderFn destroy_shader_fn;

    Walrus_RhiCreateProgramFn  create_program_fn;
    Walrus_RhiDestroyProgramFn destroy_program_fn;

    Walrus_RhiCreateUniformFn  create_uniform_fn;
    Walrus_RhiDestroyUniformFn destroy_uniform_fn;
    Walrus_RhiUpdateUniformFn  update_uniform_fn;

} Walrus_RhiVTable;

typedef struct {
    char              *name;
    Walrus_UniformType type;
    u32                size;
    u32                ref_count;
} Walrus_UniformRef;

typedef struct {
    Walrus_RhiFlag flags;

    Walrus_RenderDraw    draw;
    Walrus_RenderCompute compute;
    Walrus_RenderFrame   submit_frame;

    Walrus_RenderView views[WR_RHI_MAX_VIEWS];

    Walrus_HandleAlloc *shaders;
    Walrus_HandleAlloc *programs;
    Walrus_HandleAlloc *uniforms;
    Walrus_UniformRef   uniform_refs[WR_RHI_MAX_UNIFORMS];

    Walrus_HashTable *uniform_map;
    u32 uniform_begin;
    u32 uniform_end;

    Walrus_RhiError err;
    char const     *err_msg;
} Walrus_RhiContext;

#define WR_RHI_ALLOC_FAIL_STR    "Rhi fail to allocate memory"
#define WR_RHI_GL_ALLOC_FAIL_STR "Rhi's opengl context fail to allocate memory"

void renderer_update_uniforms(UniformBuffer *uniform, u32 begin, u32 end);

void gl_backend_init(Walrus_RhiContext *ctx, Walrus_RhiVTable *vtable);

void gl_backend_shutdown(void);
