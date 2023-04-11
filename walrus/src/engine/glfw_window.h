#pragma once

#include <core/type.h>

void *glfw_create_window(char const *title, u32 width, u32 height, u32 flags);

void glfw_destroy_window(void *handle);
