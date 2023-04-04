#include <window.h>
#include <macro.h>
#include <platform.h>

#include <stdlib.h>

struct _Window {
    u32 width;
    u32 height;
    u32 flags;
};

#if PLATFORM == PLATFORM_WASI
void wajs_setup_gl_context(i32 width, i32 height);
#endif

static void setup_gl_context(i32 width, i32 height)
{
#if PLATFORM == PLATFORM_WASI
    wajs_setup_gl_context(width, height);
#else
    ASSERT(false, "unimplement");
#endif
}

Window *window_create(i32 width, i32 height, i32 flags)
{
    Window *win = malloc(sizeof(Window));
    win->width  = width;
    win->height = height;
    win->flags  = flags;

    setup_gl_context(width, height);

    return win;
}

void window_destroy(Window *window)
{
    free(window);
}

u32 window_get_width(Window *win)
{
    return win->width;
}

u32 window_get_height(Window *win)
{
    return win->height;
}