#pragma once

#include <type.h>

void *glfw_create_window(i32 width, i32 height, const char *title);

void glfw_destroy_window(void *handle);
