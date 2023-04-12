#pragma once

#include <core/type.h>
#include <core/platform.h>

#if WR_PLATFORM == WR_PLATFORM_WASM
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <GL/gl.h>

#else

#include <GL/glew.h>

#endif
