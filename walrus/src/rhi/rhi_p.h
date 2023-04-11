#pragma once

#include <rhi/rhi.h>
#include <core/hash.h>

#include "frame.h"
#include "uniform_buffer.h"

typedef void (*RhiSubmitFn)(RenderFrame *frame);
typedef void (*RhiCreateShaderFn)(Walrus_ShaderType type, Walrus_ShaderHandle handle, char const *source);
typedef void (*RhiDestroyShaderFn)(Walrus_ShaderHandle handle);
typedef void (*RhiCreateProgramFn)(Walrus_ProgramHandle handle, Walrus_ShaderHandle shader0,
                                   Walrus_ShaderHandle shader1, Walrus_ShaderHandle shader2);
typedef void (*RhiDestroyProgramFn)(Walrus_ProgramHandle handle);
typedef void (*RhiCreateUniformFn)(Walrus_UniformHandle handle, const char *name, i32 size);
typedef void (*RhiDestroyUniformFn)(Walrus_UniformHandle handle);
typedef void (*RhiUpdateUniformFn)(Walrus_UniformHandle handle, u32 offset, u32 size, void const *data);

typedef struct {
    RhiSubmitFn submit_fn;

    RhiCreateShaderFn  create_shader_fn;
    RhiDestroyShaderFn destroy_shader_fn;

    RhiCreateProgramFn  create_program_fn;
    RhiDestroyProgramFn destroy_program_fn;

    RhiCreateUniformFn  create_uniform_fn;
    RhiDestroyUniformFn destroy_uniform_fn;
    RhiUpdateUniformFn  update_uniform_fn;

} RhiVTable;

typedef struct {
    char              *name;
    Walrus_UniformType type;
    u32                size;
    u32                ref_count;
} UniformRef;

typedef struct {
    Walrus_RhiFlag flags;

    RenderDraw    draw;
    RenderCompute compute;
    RenderFrame   frames;
    RenderFrame  *submit_frame;

    RenderView views[WR_RHI_MAX_VIEWS];

    Walrus_HandleAlloc *shaders;
    Walrus_HandleAlloc *programs;
    Walrus_HandleAlloc *uniforms;
    UniformRef          uniform_refs[WR_RHI_MAX_UNIFORMS];

    Walrus_HashTable *uniform_map;
    u32               uniform_begin;
    u32               uniform_end;

    Walrus_RhiError err;
    char const     *err_msg;
} RhiContext;

typedef struct {
    enum {
        PREDEFINED_VIEW,
        PREDEFINED_VIEWPROJ,
        PREDEFINED_MODEL,

        PREDEFINED_COUNT
    } type;

    u16 loc;
} PredefinedUniform;

#define WR_RHI_ALLOC_FAIL_STR    "Rhi fail to allocate memory"
#define WR_RHI_GL_ALLOC_FAIL_STR "Rhi's opengl context fail to allocate memory"

void renderer_update_uniforms(UniformBuffer *uniform, u32 begin, u32 end);

u8 get_predefined_type(const char *name);

void gl_backend_init(RhiContext *ctx, RhiVTable *vtable);

void gl_backend_shutdown(void);
