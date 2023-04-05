#pragma once

#include <platform.h>

#if PLATFORM != PLATFORM_WASM
void glew_init(void);
#else
void wajs_setup_gl_context(void);
#endif
