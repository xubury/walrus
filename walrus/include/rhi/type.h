#pragma once

#include <core/handle_alloc.h>

#define WR_RHI_HANDLE(name) \
    typedef struct {        \
        Walrus_Handle id;   \
    } name

typedef enum {
    WR_RHI_SUCCESS = 0,

    WR_RHI_INIT_GLEW_ERROR,
    WR_RHI_ALLOC_ERROR,

    WR_RHI_ALLOC_HADNLE_ERROR,
} Walrus_RhiError;

typedef enum {
    WR_RHI_FLAG_NONE = 0,

    WR_RHI_FLAG_OPENGL = 1 << 0,
} Walrus_RhiFlag;

typedef enum {
    WR_RHI_SHADER_COMPUTE,
    WR_RHI_SHADER_VERTEX,
    WR_RHI_SHADER_GEOMETRY,
    WR_RHI_SHADER_FRAGMENT,

    WR_RHI_SHADER_COUNT
} Walrus_ShaderType;

WR_RHI_HANDLE(Walrus_ShaderHandle);
WR_RHI_HANDLE(Walrus_ProgramHandle);
