#pragma once

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <GL/gl.h>

typedef void (*WajsRenderCallback)();

#ifdef __cplusplus
extern "C" {
#endif

void wajsSetupGlCanvas(int width, int height);

void wajsSetGlRenderCallback(WajsRenderCallback callback);

float wajsGetFrameTime();

#ifdef __cplusplus
}
#endif
