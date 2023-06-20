#include <engine/window.h>
#include <core/macro.h>
#include <core/platform.h>
#include <core/memory.h>

#include "window_p.h"

#if WR_PLATFORM == WR_PLATFORM_WASM
void wajs_create_window(char const *title, u32 width, u32 height);
#else
#include "glfw_window.h"
#include <GLFW/glfw3.h>
#endif

bool walrus_window_init(Walrus_Window *win, char const *title, u32 width, u32 height, u32 flags)
{
    win->width  = width;
    win->height = height;
    win->flags  = flags;

#if WR_PLATFORM == WR_PLATFORM_WASM
    win->handle = NULL;
    wajs_create_window(title, width, height);
#else
    glfw_create_window(win, title, width, height, flags);
    if (win->handle == NULL) {
        walrus_window_shutdown(win);
        return false;
    }
#endif
    return true;
}

void walrus_window_shutdown(Walrus_Window *win)
{
#if WR_PLATFORM != WR_PLATFORM_WASM
    if (win->handle != NULL) {
        glfw_destroy_window(win->handle);
    }
#endif
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
