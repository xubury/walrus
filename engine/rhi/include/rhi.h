#pragma once

#include "gl.h"

typedef enum {
    RHI_SUCCESS = 0,

    RHI_INIT_GLEW_ERROR
} RhiError;

typedef enum {
    RHI_FLAG_NONE           = 0,
    RHI_FLAG_BACKEND_OPENGL = 1 << 0,
} RhiFlag;

RhiError rhi_init(RhiFlag flags);

char const *rhi_error_msg(void);

void rhi_set_resolution(i32 width, i32 height);

void rhi_frame(void);
