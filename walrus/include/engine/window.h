#pragma once

#include <core/type.h>

typedef struct _Walrus_Window Walrus_Window;

typedef enum {
    WR_WINDOW_FLAG_NONE = 0,

    WR_WINDOW_FLAG_ASYNC = 1 << 0,

    WR_WINDOW_FLAG_OPENGL = 1 << 1
} WindowFlag;

Walrus_Window *walrus_window_create(char const *title, i32 width, i32 height, i32 flags);

void walrus_window_destroy(Walrus_Window *win);

u32 walrus_window_width(Walrus_Window *win);

u32 walrus_window_height(Walrus_Window *win);

void walrus_window_swap_buffers(Walrus_Window *win);

void walrus_window_poll_events(Walrus_Window *win);
