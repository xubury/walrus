#include <window.h>
#include <macro.h>
#include <platform.h>
#include <rhi.h>
#include <stdlib.h>

struct _Window {
    u32   width;
    u32   height;
    u32   flags;
    void *handle;
};

#if PLATFORM == PLATFORM_WASM
void wajs_create_window(i32 width, i32 height);
#else
#include <glfw_window.h>
#include <GLFW/glfw3.h>
#endif

static void setup_gl_context(Window *win, i32 width, i32 height)
{
#if PLATFORM == PLATFORM_WASM
    win->handle = NULL;
    wajs_create_window(width, height);
#else
    win->handle = glfw_create_window(width, height, "null");
    if (win->handle == NULL) {
        printf("Error creating window!\n");
    }
#endif
}

static void release_gl_context(Window *win)
{
#if PLATFORM != PLATFORM_WASM
    if (win->handle != NULL) {
        glfw_destroy_window(win->handle);
    }
#else
    UNUSED(win)
#endif
}

Window *window_create(i32 width, i32 height, i32 flags)
{
    Window *win = malloc(sizeof(Window));
    win->width  = width;
    win->height = height;
    win->flags  = flags;
    win->handle = NULL;

    setup_gl_context(win, width, height);

#if PLATFORM != PLATFORM_WASM
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
        release_gl_context(win);
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
