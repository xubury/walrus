#pragma once

#include <rhi.h>

typedef struct {
    u32 width;
    u32 height;
} RhiResolution;

typedef struct {
    RhiResolution resolution;
} RhiFrame;

typedef void (*RhiSubmitFn)(RhiFrame *frame);

typedef struct {
    RhiFlag flags;

    RhiFrame submit_frame;

    RhiSubmitFn submit_fn;

    RhiError    err;
    char const *err_msg;
} Rhi;

void init_gl_backend(Rhi *rhi);
