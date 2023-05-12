#pragma once

#include <core/type.h>

typedef struct _Walrus_Window Walrus_Window;

typedef enum {
    WR_WINDOW_FLAG_NONE = 0,

    WR_WINDOW_FLAG_VSYNC      = 1 << 0,
    WR_WINDOW_FLAG_OPENGL     = 1 << 1,
    WR_WINDOW_FLAG_RESIZABLE = 1 << 2
} WindowFlag;

Walrus_Window *walrus_window_create(char const *title, u32 width, u32 height, u32 flags);

void walrus_window_destroy(Walrus_Window *win);

u32 walrus_window_width(Walrus_Window *win);

u32 walrus_window_height(Walrus_Window *win);

void walrus_window_swap_buffers(Walrus_Window *win);

void walrus_window_poll_events(Walrus_Window *win);

void walrus_window_make_current(Walrus_Window *win);

void walrus_window_set_vsync(Walrus_Window *win, bool vsync);
