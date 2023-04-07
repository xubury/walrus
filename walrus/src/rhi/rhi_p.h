#pragma once

#include <rhi/rhi.h>

typedef struct {
    u32 width;
    u32 height;
} RhiResolution;

typedef struct {
    RhiResolution resolution;
} RhiFrame;

typedef void (*RhiSubmitFn)(RhiFrame *frame);

typedef struct {
    Walrus_RhiFlag flags;

    RhiFrame submit_frame;

    RhiSubmitFn submit_fn;

    Walrus_RhiError    err;
    char const *err_msg;
} Rhi;

void init_gl_backend(Rhi *rhi);
