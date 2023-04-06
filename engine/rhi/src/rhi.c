#include <rhi.h>

GLenum glew_init(void);

typedef struct {
    RhiError rhi_err;
    GLenum   glew_err;
} Rhi;

static Rhi s_rhi;

RhiError rhi_init(void)
{
#if PLATFORM != PLATFORM_WASM
    s_rhi.glew_err = glew_init();
    if (s_rhi.glew_err != GLEW_OK) {
        s_rhi.rhi_err = RHI_INIT_GLEW_ERROR;
    }
#else
    wajs_setup_gl_context();
#endif
    return s_rhi.rhi_err;
}

char const *rhi_error_msg(void)
{
#if PLATFORM != PLATFORM_WASM
    if (s_rhi.rhi_err == RHI_INIT_GLEW_ERROR) {
        return (char const *)glewGetErrorString(s_rhi.glew_err);
    }
#endif
    return "No Error";
}
