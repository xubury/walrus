#pragma once

#include <type.h>

typedef struct _Window Window;

Window *window_create(i32 width, i32 height, i32 flags);

void window_destroy(Window *win);

u32 window_get_width(Window *win);

u32 window_get_height(Window *win);
