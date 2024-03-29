#pragma once

#include <core/hash.h>
#include <core/semaphore.h>
#include <core/cpoly.h>

#include "frame.h"
#include "uniform_buffer.h"

typedef struct {
    POLY_TABLE(Renderer, POLY_INTERFACE(init), POLY_INTERFACE(shutdown), POLY_INTERFACE(submit),
               POLY_INTERFACE(create_shader), POLY_INTERFACE(destroy_shader), POLY_INTERFACE(create_program),
               POLY_INTERFACE(destroy_program), POLY_INTERFACE(create_uniform), POLY_INTERFACE(destroy_uniform),
               POLY_INTERFACE(resize_uniform), POLY_INTERFACE(update_uniform), POLY_INTERFACE(create_vertex_layout),
               POLY_INTERFACE(destroy_vertex_layout), POLY_INTERFACE(create_buffer), POLY_INTERFACE(destroy_buffer),
               POLY_INTERFACE(update_buffer), POLY_INTERFACE(create_texture), POLY_INTERFACE(destroy_texture),
               POLY_INTERFACE(resize_texture), POLY_INTERFACE(create_framebuffer), POLY_INTERFACE(destroy_framebuffer))
} Renderer;

POLY_PROTOTYPE(void, init, Renderer *renderer, Walrus_RhiCreateInfo const *info, Walrus_RhiCapabilities *caps)
POLY_PROTOTYPE(void, shutdown, void)

POLY_PROTOTYPE(void, submit, RenderFrame *frame)

POLY_PROTOTYPE(void, create_shader, Walrus_ShaderType type, Walrus_ShaderHandle handle, char const *source)
POLY_PROTOTYPE(void, destroy_shader, Walrus_ShaderHandle handle)

POLY_PROTOTYPE(void, create_program, Walrus_ProgramHandle handle, Walrus_ShaderHandle *shaders, u32 num)
POLY_PROTOTYPE(void, destroy_program, Walrus_ProgramHandle handle)

POLY_PROTOTYPE(void, create_uniform, Walrus_UniformHandle handle, const char *name, u32 size)
POLY_PROTOTYPE(void, destroy_uniform, Walrus_UniformHandle handle)
POLY_PROTOTYPE(void, resize_uniform, Walrus_UniformHandle handle, u32 size)
POLY_PROTOTYPE(void, update_uniform, Walrus_UniformHandle handle, u32 offset, u32 size, void const *data)

POLY_PROTOTYPE(void, create_vertex_layout, Walrus_LayoutHandle handle, Walrus_VertexLayout const *layout)
POLY_PROTOTYPE(void, destroy_vertex_layout, Walrus_LayoutHandle handle)

POLY_PROTOTYPE(void, create_buffer, Walrus_BufferHandle handle, void const *data, u64 size, u16 flags)
POLY_PROTOTYPE(void, destroy_buffer, Walrus_BufferHandle handle)
POLY_PROTOTYPE(void, update_buffer, Walrus_BufferHandle handle, u64 offset, u64 size, void const *data)

POLY_PROTOTYPE(void, create_texture, Walrus_TextureHandle handle, Walrus_TextureCreateInfo const *info,
               void const *data)
POLY_PROTOTYPE(void, destroy_texture, Walrus_TextureHandle handle)
POLY_PROTOTYPE(void, resize_texture, Walrus_TextureHandle handle, u32 width, u32 height, u32 depth, u8 num_mipmaps,
               u8 num_layers)
POLY_PROTOTYPE(void, create_framebuffer, Walrus_FramebufferHandle handle, Walrus_Attachment *attachments, u8 num)
POLY_PROTOTYPE(void, destroy_framebuffer, Walrus_FramebufferHandle handle)

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
    u32                 num;
    Walrus_ShaderHandle shaders[3];
} ProgramRef;

typedef struct {
    Walrus_BackBufferRatio ratio;
    Walrus_TextureHandle   handle;
    u32                    width;
    u32                    height;
    u32                    depth;
    u8                     num_layers;
    u8                     num_mipmaps;
    u32                    ref_count;
} TextureRef;

typedef struct {
    uint32_t             width;
    uint32_t             height;
    Walrus_TextureHandle th[WR_RHI_MAX_FRAMEBUFFER_ATTACHMENTS];
} FramebufferRef;

typedef struct {
    bool initialized;
    bool exit;

    Walrus_RhiCreateInfo info;

    Walrus_RhiCapabilities caps;

    Walrus_Semaphore *api_sem;
    Walrus_Semaphore *render_sem;

    Walrus_Resolution resolution;

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
    ProgramRef          program_refs[WR_RHI_MAX_PROGRAMS];

    Walrus_HandleAlloc *uniforms;
    UniformRef          uniform_refs[WR_RHI_MAX_UNIFORMS];
    Walrus_HandleAlloc *vertex_layouts;
    Walrus_HandleAlloc *buffers;
    Walrus_HandleAlloc *textures;
    TextureRef          texture_refs[WR_RHI_MAX_TEXTURES];

    Walrus_HandleAlloc *framebuffers;
    FramebufferRef      fb_refs[WR_RHI_MAX_FRAMEBUFFERS];

    Walrus_HashTable *uniform_map;
    u32               uniform_begin;
    u32               uniform_end;

    VertexLayoutRef   vertex_layout_ref[WR_RHI_MAX_VERTEX_LAYOUTS];
    Walrus_HashTable *vertex_layout_table;

    Walrus_RhiError err;
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

void renderer_uniform_updates(UniformBuffer *uniform, u32 begin, u32 end);

u8 get_predefined_type(char const *name);

char const *get_glsl_header(void);

WR_INLINE u64 pack_stencil(u64 fstencil, u64 bstencil)
{
    return (fstencil << 32) | bstencil;
}

WR_INLINE u32 unpack_stencil(bool zero, u64 pack)
{
    return pack >> (zero ? 0 : 32);
}

WR_INLINE u32 pack_rgba(u8 r, u8 g, u8 b, u8 a)
{
    return (u32)(r) << 24 | (u32)(g) << 16 | (u32)(b) << 8 | (u32)(a) << 0;
}

WR_INLINE void unpack_rgba(u32 rgba, u8 *r, u8 *g, u8 *b, u8 *a)
{
    *r = (u8)(rgba >> 24);
    *g = (u8)(rgba >> 16);
    *b = (u8)(rgba >> 8);
    *a = (u8)(rgba >> 0);
}
