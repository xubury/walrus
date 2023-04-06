#include "rhi_p.h"

#include <macro.h>
#include <log.h>

static void submit(RhiFrame *frame)
{
    UNUSED(frame);
}

void init_gl_backend(Rhi *rhi)
{
    if (rhi) {
        rhi->submit_fn = submit;
    }
}
