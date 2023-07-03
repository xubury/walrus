#pragma once

#include "rhi_p.h"
#include "uniform_buffer.h"
#include "gl.h"

#include <core/hash.h>

typedef struct {
    GLuint            id;
    UniformBuffer    *buffer;
    u8                num_predefineds;
    PredefinedUniform predefineds[PREDEFINED_COUNT];
} GlProgram;

typedef struct {
    GLuint id;
    u64    size;
    GLenum target;
    u16    flags;
} GlBuffer;

typedef struct {
    GLenum internal_format;
    GLenum internal_srgb_format;
    GLenum format;
    GLenum type;
} GlFormat;

typedef struct {
    GLuint id;
    GLenum rbo;
    GLenum target;

    u32                width;
    u32                height;
    u32                depth;
    u64                flags;
    u32                num_mipmaps;
    Walrus_PixelFormat format;

    GlFormat gl;

} GlTexture;

typedef struct {
    GLuint            fbo[2];
    u32               width;
    u32               height;
    u8                num_textures;
    Walrus_Attachment attachments[WR_RHI_MAX_FRAMEBUFFER_ATTACHMENTS];
} GlFramebuffer;

typedef struct {
    GLuint vao;
    GLuint current_fbo;
    GLuint msaa_fbo;
    GLuint msaa_rbos[2];

    Walrus_FramebufferHandle fbo;
    Walrus_Resolution        resolution;

    u32 discards;

    GlBuffer buffers[WR_RHI_MAX_BUFFERS];

    GLuint    shaders[WR_RHI_MAX_SHADERS];
    GlProgram programs[WR_RHI_MAX_PROGRAMS];

    void             *uniforms[WR_RHI_MAX_UNIFORMS];
    char             *uniform_names[WR_RHI_MAX_UNIFORMS];
    Walrus_HashTable *uniform_registry;

    Walrus_VertexLayout vertex_layouts[WR_RHI_MAX_VERTEX_LAYOUTS];

    GlTexture textures[WR_RHI_MAX_TEXTURES];

    GlFramebuffer framebuffers[WR_RHI_MAX_FRAMEBUFFERS];
} GlContext;

extern GlContext *gl_ctx;
