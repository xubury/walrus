#pragma once

#include <type.h>

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
// #define GL_VERSION_3_0 1
// #define GL_VERSION_3_1 1
// #define GL_VERSION_3_2 1
// #define GL_VERSION_3_3 1
#define GL_VERSION_4_0 0
#define GL_VERSION_4_1 0
#define GL_VERSION_4_2 0
#define GL_VERSION_4_3 0
#define GL_VERSION_4_4 0
#define GL_VERSION_4_5 0
#include <GL/gl.h>

typedef void (*WajsLoopCallback)(void);

void wajs_setup_gl_context(i32 width, i32 height);

void wajs_set_main_loop(WajsLoopCallback callback);
