#include <engine/window.h>
#include <core/macro.h>
#include <core/platform.h>

#include <stdlib.h>

struct _Window {
    u32   width;
    u32   height;
    u32   flags;
    void *handle;
};

#if PLATFORM == PLATFORM_WASM
void wajs_create_window(char const *title, i32 width, i32 height);
#else
#include <engine/glfw_window.h>
#include <GLFW/glfw3.h>
#endif

Window *window_create(char const *title, i32 width, i32 height, i32 flags)
{
    Window *win = malloc(sizeof(Window));
    win->width  = width;
    win->height = height;
    win->flags  = flags;

#if PLATFORM == PLATFORM_WASM
    win->handle = NULL;
    wajs_create_window(title, width, height);
#else
    win->handle = glfw_create_window(title, width, height, flags);
    if (win->handle == NULL) {
        window_destroy(win);
        win = NULL;
    }
#endif
    return win;
}

void window_destroy(Window *win)
{
    if (win) {
#if PLATFORM != PLATFORM_WASM
        if (win->handle != NULL) {
            glfw_destroy_window(win->handle);
        }
#endif
        free(win);
    }
}

u32 window_get_width(Window *win)
{
    return win->width;
}

u32 window_get_height(Window *win)
{
    return win->height;
}

void window_poll_events(Window *win)
{
#if PLATFORM != PLATFORM_WASM
    if (win != NULL) {
        glfwPollEvents();
    }
#else
    UNUSED(win);
#endif
}

void window_swap_buffers(Window *win)
{
#if PLATFORM != PLATFORM_WASM
    if (win != NULL) {
        glfwSwapBuffers(win->handle);
    }
#else
    UNUSED(win);
#endif
}
