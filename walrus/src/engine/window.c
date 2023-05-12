#include <engine/window.h>
#include <core/macro.h>
#include <core/platform.h>
#include <core/memory.h>

struct _Walrus_Window {
    u32   width;
    u32   height;
    u32   flags;
    void *handle;
};

#if WR_PLATFORM == WR_PLATFORM_WASM
void wajs_create_window(char const *title, u32 width, u32 height);
#else
#include "glfw_window.h"
#include <GLFW/glfw3.h>
#endif

Walrus_Window *walrus_window_create(char const *title, u32 width, u32 height, u32 flags)
{
    Walrus_Window *win = walrus_malloc(sizeof(Walrus_Window));
    win->width         = width;
    win->height        = height;
    win->flags         = flags;

#if WR_PLATFORM == WR_PLATFORM_WASM
    win->handle = NULL;
    wajs_create_window(title, width, height);
#else
    win->handle = glfw_create_window(title, width, height, flags);
    if (win->handle == NULL) {
        walrus_window_destroy(win);
        win = NULL;
    }
#endif
    return win;
}

void walrus_window_destroy(Walrus_Window *win)
{
    if (win) {
#if WR_PLATFORM != WR_PLATFORM_WASM
        if (win->handle != NULL) {
            glfw_destroy_window(win->handle);
        }
#endif
        walrus_free(win);
    }
}

u32 walrus_window_width(Walrus_Window *win)
{
    return win->width;
}

u32 walrus_window_height(Walrus_Window *win)
{
    return win->height;
}

void walrus_window_poll_events(Walrus_Window *win)
{
#if WR_PLATFORM != WR_PLATFORM_WASM
    if (win != NULL) {
        glfwPollEvents();
    }
#else
    walrus_unused(win);
#endif
}

void walrus_window_swap_buffers(Walrus_Window *win)
{
#if WR_PLATFORM != WR_PLATFORM_WASM
    if (win != NULL) {
        glfwSwapBuffers(win->handle);
    }
#else
    walrus_unused(win);
#endif
}

void walrus_window_make_current(Walrus_Window *win)
{
#if WR_PLATFORM != WR_PLATFORM_WASM
    if (win != NULL) {
        glfwMakeContextCurrent(win->handle);
    }
#else
    walrus_unused(win);
#endif
}

void walrus_window_set_vsync(Walrus_Window *win, bool vsync)
{
#if WR_PLATFORM != WR_PLATFORM_WASM
    if (win != NULL) {
        glfwSwapInterval(vsync ? 1 : 0);
    }
#else
    walrus_unused(win);
#endif
}
