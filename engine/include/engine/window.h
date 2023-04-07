#pragma once

#include <core/type.h>

typedef struct _Window Window;

typedef enum {
    WINDOW_FLAG_NONE  = 0,
    WINDOW_FLAG_ASYNC = 1 << 0
} WindowFlag;

Window *window_create(char const *title, i32 width, i32 height, i32 flags);

void window_destroy(Window *win);

u32 window_get_width(Window *win);

u32 window_get_height(Window *win);

void window_swap_buffers(Window *win);

void window_poll_events(Window *win);
