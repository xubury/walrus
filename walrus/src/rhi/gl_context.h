#pragma once

#include "rhi_p.h"
#include "uniform_buffer.h"

#include <core/hash.h>

typedef struct {
    GLuint            id;
    UniformBuffer    *buffer;
    u8                num_predefineds;
    PredefinedUniform predefineds[PREDEFINED_COUNT];
} GlProgram;

typedef struct {
    GLuint            vao;
    GLuint            shaders[WR_RHI_MAX_SHADERS];
    GlProgram         programs[WR_RHI_MAX_PROGRAMS];
    void             *uniforms[WR_RHI_MAX_UNIFORMS];
    char             *uniform_names[WR_RHI_MAX_UNIFORMS];
    Walrus_HashTable *uniform_registry;
} GlContext;

extern GlContext *g_ctx;
