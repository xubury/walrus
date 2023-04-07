#include "rhi_p.h"

#include <core/macro.h>
#include <core/log.h>

static void submit(RhiFrame *frame)
{
    walrus_unused(frame);
}

void init_gl_backend(Rhi *rhi)
{
    if (rhi) {
        rhi->submit_fn = submit;
    }
}
