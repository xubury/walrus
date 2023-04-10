#pragma once

#include "rhi_p.h"

typedef struct {
    GLuint vao;
    GLuint shaders[WR_RHI_MAX_SHADERS];
    GLuint programs[WR_RHI_MAX_PROGRAMS];
} Walrus_GlContext;

extern Walrus_GlContext *g_ctx;
