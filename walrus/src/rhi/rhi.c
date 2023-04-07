#include "rhi_p.h"

#include <core/macro.h>

GLenum glew_init(void);

static Rhi s_rhi;

Walrus_RhiError walrus_rhi_init(Walrus_RhiFlag flags)
{
    s_rhi.err_msg = "No error";
    s_rhi.flags   = flags;

#if WR_PLATFORM != WR_PLATFORM_WASM
    GLenum err = glew_init();
    if (err != GLEW_OK) {
        s_rhi.err     = WR_RHI_INIT_GLEW_ERROR;
        s_rhi.err_msg = (char const *)glewGetErrorString(err);
    }
#else
    wajs_setup_gl_context();
#endif

    if (s_rhi.flags & WR_RHI_FLAG_OPENGL) {
        init_gl_backend(&s_rhi);
    }
    else {
        walrus_assert_msg(false, "No render backend specifed!");
    }
    return s_rhi.err;
}

char const *walrus_rhi_error_msg(void)
{
    return s_rhi.err_msg;
}

void walrus_rhi_set_resolution(i32 width, i32 height)
{
    s_rhi.submit_frame.resolution.width  = width;
    s_rhi.submit_frame.resolution.height = height;
}

void walrus_rhi_frame(void)
{
    s_rhi.submit_fn(&s_rhi.submit_frame);
}
