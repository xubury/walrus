#include "rhi_p.h"

#include <core/macro.h>

GLenum glew_init(void);

static Rhi s_rhi;

RhiError rhi_init(RhiFlag flags)
{
    s_rhi.err_msg = "No error";
    s_rhi.flags   = flags;

#if PLATFORM != PLATFORM_WASM
    GLenum err = glew_init();
    if (err != GLEW_OK) {
        s_rhi.err     = RHI_INIT_GLEW_ERROR;
        s_rhi.err_msg = (char const *)glewGetErrorString(err);
    }
#else
    wajs_setup_gl_context();
#endif

    if (s_rhi.flags & RHI_FLAG_BACKEND_OPENGL) {
        init_gl_backend(&s_rhi);
    }
    else {
        ASSERT_MSG(false, "No render backend specifed!");
    }
    return s_rhi.err;
}

char const *rhi_error_msg(void)
{
    return s_rhi.err_msg;
}

void rhi_set_resolution(i32 width, i32 height)
{
    s_rhi.submit_frame.resolution.width  = width;
    s_rhi.submit_frame.resolution.height = height;
}

void rhi_frame(void)
{
    s_rhi.submit_fn(&s_rhi.submit_frame);
}
