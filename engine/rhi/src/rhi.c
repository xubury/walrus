#include <rhi.h>

void glew_init(void);

void rhi_init(void)
{
#if PLATFORM != PLATFORM_WASM
    glew_init();
#else
    wajs_setup_gl_context();
#endif
}
