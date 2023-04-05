#pragma once

#include <type.h>

void *glfw_create_gl_context(i32 width, i32 height);

void glfw_destroy_gl_context(void *handle);
