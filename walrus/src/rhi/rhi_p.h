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
} Walrus_Rhi;

void init_gl_backend(Walrus_RhiVTable *vtable);
