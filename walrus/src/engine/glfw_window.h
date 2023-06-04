#pragma once

#include <core/type.h>
#include <engine/window.h>
#include "window_p.h"

void glfw_create_window(Walrus_Window *window, char const *title, u32 width, u32 height, u32 flags);

void glfw_destroy_window(void *handle);
