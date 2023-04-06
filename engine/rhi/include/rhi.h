#pragma once

#include "gl.h"

typedef enum {
    RHI_SUCCESS = 0,

    RHI_INIT_GLEW_ERROR
} RhiError;

RhiError rhi_init(void);

char const *rhi_error_msg(void);
