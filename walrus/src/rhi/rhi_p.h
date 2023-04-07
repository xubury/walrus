#pragma once

#include <rhi/rhi.h>

#include "frame.h"

typedef void (*Walrus_RhiSubmitFn)(Walrus_RenderFrame *frame);

typedef struct {
    Walrus_RhiSubmitFn submit_fn;
} Walrus_RhiVTable;

typedef struct {
    Walrus_RhiFlag flags;

    Walrus_RenderDraw    draw;
    Walrus_RenderCompute compute;
    Walrus_RenderFrame   submit_frame;

    Walrus_RenderView views[WR_RHI_MAX_VIEWS];

    Walrus_RhiError err;
    char const     *err_msg;
} Walrus_RhiContext;

#define WR_RHI_ALLOC_FAIL_STR    "Rhi fail to allocate memory"
#define WR_RHI_GL_ALLOC_FAIL_STR "Rhi's opengl context fail to allocate memory"

void init_gl_backend(Walrus_RhiContext *ctx, Walrus_RhiVTable *vtable);

void shutdown_gl_backend(void);
