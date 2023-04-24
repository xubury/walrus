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
    Walrus_PixelFormat format;

    GlFormat gl;

} GlTexture;

typedef struct {
    GLuint vao;

    GlBuffer buffers[WR_RHI_MAX_BUFFERS];

    GLuint    shaders[WR_RHI_MAX_SHADERS];
    GlProgram programs[WR_RHI_MAX_PROGRAMS];

    void             *uniforms[WR_RHI_MAX_UNIFORMS];
    char             *uniform_names[WR_RHI_MAX_UNIFORMS];
    Walrus_HashTable *uniform_registry;

    Walrus_VertexLayout vertex_layouts[WR_RHI_MAX_VERTEX_LAYOUTS];

    GlTexture textures[WR_RHI_MAX_TEXTURES];
} GlContext;

extern GlContext *g_ctx;
