#pragma once

#include <type.h>

void *glfw_create_window(char const *title, i32 width, i32 height, i32 flags);

void glfw_destroy_window(void *handle);
