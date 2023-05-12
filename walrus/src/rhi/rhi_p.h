#pragma once

#include <rhi/rhi.h>
#include <core/hash.h>
#include <core/semaphore.h>

#include "frame.h"
#include "uniform_buffer.h"

typedef void (*RhiSubmitFn)(RenderFrame *frame);

typedef void (*RhiCreateShaderFn)(Walrus_ShaderType type, Walrus_ShaderHandle handle, char const *source);
typedef void (*RhiDestroyShaderFn)(Walrus_ShaderHandle handle);
typedef void (*RhiCreateProgramFn)(Walrus_ProgramHandle handle, Walrus_ShaderHandle *shaders, u32 num);
typedef void (*RhiDestroyProgramFn)(Walrus_ProgramHandle handle);

typedef void (*RhiCreateUniformFn)(Walrus_UniformHandle handle, const char *name, u32 size);
typedef void (*RhiDestroyUniformFn)(Walrus_UniformHandle handle);
typedef void (*RhiResizeUniformFn)(Walrus_UniformHandle handle, u32 size);
typedef void (*RhiUpdateUniformFn)(Walrus_UniformHandle handle, u32 offset, u32 size, void const *data);

typedef void (*RhiCreateVertexLayoutFn)(Walrus_LayoutHandle handle, Walrus_VertexLayout const *layout);
typedef void (*RhiDestroyVertexLayoutFn)(Walrus_LayoutHandle handle);

typedef void (*RhiCreateBufferFn)(Walrus_BufferHandle handle, void const *data, u64 size, u16 flags);
typedef void (*RhiDestroyBufferFn)(Walrus_BufferHandle handle);
typedef void (*RhiUpdateBufferFn)(Walrus_BufferHandle handle, u64 offset, u64 size, void const *data);

typedef void (*RhiCreateTextureFn)(Walrus_TextureHandle handle, Walrus_TextureCreateInfo const *info, void const *data);
typedef void (*RhiDestroyTextureFn)(Walrus_TextureHandle handle);
typedef void (*RhiResizeTextureFn)(Walrus_TextureHandle handle, u32 width, u32 height, u32 depth, u8 num_layers,
                                   u8 num_mipmaps);

typedef struct {
    RhiSubmitFn submit_fn;

    RhiCreateShaderFn  shader_create_fn;
    RhiDestroyShaderFn shader_destroy_fn;

    RhiCreateProgramFn  program_create_fn;
    RhiDestroyProgramFn program_destroy_fn;

    RhiCreateUniformFn  uniform_create_fn;
    RhiDestroyUniformFn uniform_destroy_fn;
    RhiResizeUniformFn  uniform_resize_fn;
    RhiUpdateUniformFn  uniform_update_fn;

    RhiCreateVertexLayoutFn  vertex_layout_create_fn;
    RhiDestroyVertexLayoutFn vertex_layout_destroy_fn;

    RhiCreateBufferFn  buffer_create_fn;
    RhiDestroyBufferFn buffer_destroy_fn;
    RhiUpdateBufferFn  buffer_update_fn;

    RhiCreateTextureFn  texture_create_fn;
    RhiDestroyTextureFn texture_destroy_fn;
    RhiResizeTextureFn  texture_resize_fn;
} RhiRenderer;

typedef struct {
    char              *name;
    Walrus_UniformType type;
    u32                size;
    u32                ref_count;
} UniformRef;

typedef struct {
    u32 hash;
    u32 ref_count;
} VertexLayoutRef;

typedef struct {
    char *source;
    u32   ref_count;
} ShaderRef;

typedef struct {
    Walrus_BackBufferRatio ratio;
    u32                    width;
    u32                    height;
    u32                    depth;
    u8                     num_layers;
    u8                     num_mipmaps;
} TextureRef;

typedef struct {
    bool initialized;
    bool exit;

    Walrus_RhiCreateInfo info;

    Walrus_Semaphore *api_sem;
    Walrus_Semaphore *render_sem;

    Resolution resolution;

    RenderDraw    draw;
    RenderCompute compute;
    RenderBind    bind;

    Sortkey      key;
    RenderFrame  frames[2];
    RenderFrame *submit_frame;
    RenderFrame *render_frame;

    RenderView views[WR_RHI_MAX_VIEWS];
    u32        seqs[WR_RHI_MAX_VIEWS];
    u16        view_map[WR_RHI_MAX_VIEWS];

    u32 num_vertices[WR_RHI_MAX_VERTEX_STREAM];

    Walrus_HandleAlloc *shaders;
    Walrus_HandleAlloc *programs;
    Walrus_HashTable   *shader_map;
    ShaderRef           shader_refs[WR_RHI_MAX_SHADERS];

    Walrus_HandleAlloc *uniforms;
    UniformRef          uniform_refs[WR_RHI_MAX_UNIFORMS];
    Walrus_HandleAlloc *vertex_layouts;
    Walrus_HandleAlloc *buffers;
    Walrus_HandleAlloc *textures;
    TextureRef          texture_refs[WR_RHI_MAX_TEXTURES];

    Walrus_HashTable *uniform_map;
    u32               uniform_begin;
    u32               uniform_end;

    VertexLayoutRef   vertex_layout_ref[WR_RHI_MAX_VERTEX_LAYOUTS];
    Walrus_HashTable *vertex_layout_table;

    Walrus_RhiError err;
    char const     *err_msg;
} RhiContext;

typedef struct {
    enum {
        PREDEFINED_VIEW,
        PREDEFINED_VIEWPROJ,
        PREDEFINED_PROJECTION,
        PREDEFINED_MODEL,

        PREDEFINED_COUNT
    } type;

    u16 loc;
} PredefinedUniform;

#define WR_RHI_ALLOC_FAIL_STR    "Rhi fail to allocate memory"
#define WR_RHI_GL_ALLOC_FAIL_STR "Rhi opengl context fail to allocate memory"

void renderer_uniform_updates(UniformBuffer *uniform, u32 begin, u32 end);

u8 get_predefined_type(char const *name);

char const *get_glsl_header(void);

void gl_backend_init(RhiContext *ctx, RhiRenderer *vtable);

void gl_backend_shutdown(void);
