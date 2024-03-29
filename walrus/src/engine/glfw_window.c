#include "glfw_window.h"
#include "window_private.h"

#include <engine/window.h>
#include <engine/event.h>
#include <engine/input.h>

#include <core/macro.h>
#include <core/assert.h>
#include <core/log.h>

#include <GLFW/glfw3.h>

static void error_callback(i32 error, char const *description)
{
    walrus_error("GLFW Error ({%d}): {%s}\n", error, description);
}

static void window_size_callback(GLFWwindow *window, i32 width, i32 height)
{
    Walrus_Window *win = (Walrus_Window *)glfwGetWindowUserPointer(window);
    win->width         = width;
    win->height        = height;
}

static void framebuffer_size_callback(GLFWwindow *window, i32 width, i32 height)
{
    walrus_unused(window);
    if (width == 1 && height == 1) {
        // minimize callback?
        return;
    }
    Walrus_Event e;
    e.type              = WR_EVENT_TYPE_RESOLUTION;
    e.resolution.width  = width;
    e.resolution.height = height;
    walrus_event_push(&e);
}

static void window_close_callback(GLFWwindow *window)
{
    walrus_unused(window);
    Walrus_Event e;
    e.type = WR_EVENT_TYPE_EXIT;
    walrus_event_push(&e);
}

static u16 translate_key(i32 keycode)
{
    switch (keycode) {
        case GLFW_KEY_TAB:
            return WR_KEY_TAB;
        case GLFW_KEY_LEFT:
            return WR_KEY_LEFT_ARROW;
        case GLFW_KEY_RIGHT:
            return WR_KEY_RIGHT_ARROW;
        case GLFW_KEY_UP:
            return WR_KEY_UP_ARROW;
        case GLFW_KEY_DOWN:
            return WR_KEY_DOWN_ARROW;
        case GLFW_KEY_PAGE_UP:
            return WR_KEY_PAGE_UP;
        case GLFW_KEY_PAGE_DOWN:
            return WR_KEY_PAGE_DOWN;
        case GLFW_KEY_HOME:
            return WR_KEY_HOME;
        case GLFW_KEY_END:
            return WR_KEY_END;
        case GLFW_KEY_INSERT:
            return WR_KEY_INSERT;
        case GLFW_KEY_DELETE:
            return WR_KEY_DELETE;
        case GLFW_KEY_BACKSPACE:
            return WR_KEY_BACKSPACE;
        case GLFW_KEY_SPACE:
            return WR_KEY_SPACE;
        case GLFW_KEY_ENTER:
            return WR_KEY_ENTER;
        case GLFW_KEY_ESCAPE:
            return WR_KEY_ESCAPE;
        case GLFW_KEY_APOSTROPHE:
            return WR_KEY_QUOTE;
        case GLFW_KEY_COMMA:
            return WR_KEY_COMMA;
        case GLFW_KEY_MINUS:
            return WR_KEY_MINUS;
        case GLFW_KEY_PERIOD:
            return WR_KEY_PERIOD;
        case GLFW_KEY_SLASH:
            return WR_KEY_SLASH;
        case GLFW_KEY_SEMICOLON:
            return WR_KEY_SEMICOLON;
        case GLFW_KEY_EQUAL:
            return WR_KEY_EQUAL;
        case GLFW_KEY_LEFT_BRACKET:
            return WR_KEY_LEFT_BRACKET;
        case GLFW_KEY_BACKSLASH:
            return WR_KEY_BACK_SLASH;
        case GLFW_KEY_RIGHT_BRACKET:
            return WR_KEY_RIGHT_BRACKET;
        case GLFW_KEY_GRAVE_ACCENT:
            return WR_KEY_BACK_QUOTE;
        case GLFW_KEY_CAPS_LOCK:
            return WR_KEY_CAPS_LOCK;
        case GLFW_KEY_SCROLL_LOCK:
            return WR_KEY_SCROLL_LOCK;
        case GLFW_KEY_NUM_LOCK:
            return WR_KEY_NUM_LOCK;
        case GLFW_KEY_PRINT_SCREEN:
            return WR_KEY_PRINT_SCREEN;
        case GLFW_KEY_PAUSE:
            return WR_KEY_PAUSE;
        case GLFW_KEY_KP_0:
            return WR_KEY_KEYPAD0;
        case GLFW_KEY_KP_1:
            return WR_KEY_KEYPAD1;
        case GLFW_KEY_KP_2:
            return WR_KEY_KEYPAD2;
        case GLFW_KEY_KP_3:
            return WR_KEY_KEYPAD3;
        case GLFW_KEY_KP_4:
            return WR_KEY_KEYPAD4;
        case GLFW_KEY_KP_5:
            return WR_KEY_KEYPAD5;
        case GLFW_KEY_KP_6:
            return WR_KEY_KEYPAD6;
        case GLFW_KEY_KP_7:
            return WR_KEY_KEYPAD7;
        case GLFW_KEY_KP_8:
            return WR_KEY_KEYPAD8;
        case GLFW_KEY_KP_9:
            return WR_KEY_KEYPAD9;
        case GLFW_KEY_KP_DECIMAL:
            return WR_KEY_KEYPAD_DECIMAL;
        case GLFW_KEY_KP_DIVIDE:
            return WR_KEY_KEYPAD_DIVIDE;
        case GLFW_KEY_KP_MULTIPLY:
            return WR_KEY_KEYPAD_MULTIPLY;
        case GLFW_KEY_KP_SUBTRACT:
            return WR_KEY_KEYPAD_SUBTRACT;
        case GLFW_KEY_KP_ADD:
            return WR_KEY_KEYPAD_ADD;
        case GLFW_KEY_KP_ENTER:
            return WR_KEY_KEYPAD_ENTER;
        case GLFW_KEY_KP_EQUAL:
            return WR_KEY_KEYPAD_EQUAL;
        case GLFW_KEY_LEFT_SHIFT:
            return WR_KEY_LEFT_SHIFT;
        case GLFW_KEY_LEFT_CONTROL:
            return WR_KEY_LEFT_CTRL;
        case GLFW_KEY_LEFT_ALT:
            return WR_KEY_LEFT_ALT;
        case GLFW_KEY_LEFT_SUPER:
            return WR_KEY_LEFT_SUPER;
        case GLFW_KEY_RIGHT_SHIFT:
            return WR_KEY_RIGHT_SHIFT;
        case GLFW_KEY_RIGHT_CONTROL:
            return WR_KEY_RIGHT_CTRL;
        case GLFW_KEY_RIGHT_ALT:
            return WR_KEY_RIGHT_ALT;
        case GLFW_KEY_RIGHT_SUPER:
            return WR_KEY_RIGHT_SUPER;
        case GLFW_KEY_MENU:
            return WR_KEY_MENU;
        case GLFW_KEY_0:
            return WR_KEY_D0;
        case GLFW_KEY_1:
            return WR_KEY_D1;
        case GLFW_KEY_2:
            return WR_KEY_D2;
        case GLFW_KEY_3:
            return WR_KEY_D3;
        case GLFW_KEY_4:
            return WR_KEY_D4;
        case GLFW_KEY_5:
            return WR_KEY_D5;
        case GLFW_KEY_6:
            return WR_KEY_D6;
        case GLFW_KEY_7:
            return WR_KEY_D7;
        case GLFW_KEY_8:
            return WR_KEY_D8;
        case GLFW_KEY_9:
            return WR_KEY_D9;
        case GLFW_KEY_A:
            return WR_KEY_A;
        case GLFW_KEY_B:
            return WR_KEY_B;
        case GLFW_KEY_C:
            return WR_KEY_C;
        case GLFW_KEY_D:
            return WR_KEY_D;
        case GLFW_KEY_E:
            return WR_KEY_E;
        case GLFW_KEY_F:
            return WR_KEY_F;
        case GLFW_KEY_G:
            return WR_KEY_G;
        case GLFW_KEY_H:
            return WR_KEY_H;
        case GLFW_KEY_I:
            return WR_KEY_I;
        case GLFW_KEY_J:
            return WR_KEY_J;
        case GLFW_KEY_K:
            return WR_KEY_K;
        case GLFW_KEY_L:
            return WR_KEY_L;
        case GLFW_KEY_M:
            return WR_KEY_M;
        case GLFW_KEY_N:
            return WR_KEY_N;
        case GLFW_KEY_O:
            return WR_KEY_O;
        case GLFW_KEY_P:
            return WR_KEY_P;
        case GLFW_KEY_Q:
            return WR_KEY_Q;
        case GLFW_KEY_R:
            return WR_KEY_R;
        case GLFW_KEY_S:
            return WR_KEY_S;
        case GLFW_KEY_T:
            return WR_KEY_T;
        case GLFW_KEY_U:
            return WR_KEY_U;
        case GLFW_KEY_V:
            return WR_KEY_V;
        case GLFW_KEY_W:
            return WR_KEY_W;
        case GLFW_KEY_X:
            return WR_KEY_X;
        case GLFW_KEY_Y:
            return WR_KEY_Y;
        case GLFW_KEY_Z:
            return WR_KEY_Z;
        case GLFW_KEY_F1:
            return WR_KEY_F1;
        case GLFW_KEY_F2:
            return WR_KEY_F2;
        case GLFW_KEY_F3:
            return WR_KEY_F3;
        case GLFW_KEY_F4:
            return WR_KEY_F4;
        case GLFW_KEY_F5:
            return WR_KEY_F5;
        case GLFW_KEY_F6:
            return WR_KEY_F6;
        case GLFW_KEY_F7:
            return WR_KEY_F7;
        case GLFW_KEY_F8:
            return WR_KEY_F8;
        case GLFW_KEY_F9:
            return WR_KEY_F9;
        case GLFW_KEY_F10:
            return WR_KEY_F10;
        case GLFW_KEY_F11:
            return WR_KEY_F11;
        case GLFW_KEY_F12:
            return WR_KEY_F12;
        default:
            return WR_KEY_UNKNOWN;
    }
}

static Walrus_Keymod translate_keymods(i32 mods)
{
    Walrus_Keymod res = WR_KEYMOD_NONE;

    if (GLFW_MOD_ALT & mods) {
        res |= WR_KEYMOD_ALT;
    }
    if (GLFW_MOD_CONTROL & mods) {
        res |= WR_KEYMOD_CTRL;
    }
    if (GLFW_MOD_SHIFT & mods) {
        res |= WR_KEYMOD_SHIFT;
    }
    if (GLFW_MOD_SUPER & mods) {
        res |= WR_KEYMOD_SUPER;
    }
    return res;
}

static void key_callback(GLFWwindow *window, i32 keycode, i32 scancode, i32 action, i32 mods)
{
    walrus_unused(window);
    walrus_unused(scancode);

    Walrus_Keyboard button = translate_key(keycode);
    if (button == WR_KEY_UNKNOWN) {
        walrus_error("Unknown keycode: %d", keycode);
        return;
    }
    else {
        Walrus_Event e;
        e.type          = WR_EVENT_TYPE_BUTTON;
        e.button.button = button;
        e.button.device = WR_INPUT_KEYBOARD;
        e.button.state  = action != GLFW_RELEASE;
        e.button.mods   = translate_keymods(mods);
        walrus_event_push(&e);
    }
}

static void cursor_callback(GLFWwindow *window, f64 x, f64 y)
{
    walrus_unused(window);

    Walrus_Event e;
    e.type        = WR_EVENT_TYPE_AXIS;
    e.axis.device = WR_INPUT_MOUSE;
    e.axis.axis   = WR_MOUSE_AXIS_CURSOR;
    e.axis.x      = x;
    e.axis.y      = y;
    e.axis.z      = 0;
    e.axis.mods   = 0;
    walrus_event_push(&e);
}

static void scroll_callback(GLFWwindow *window, f64 x_offset, f64 y_offset)
{
    walrus_unused(window);

    static f64 x = 0;
    static f64 y = 0;
    x += x_offset;
    y += y_offset;

    Walrus_Event e;
    e.type        = WR_EVENT_TYPE_AXIS;
    e.axis.device = WR_INPUT_MOUSE;
    e.axis.axis   = WR_MOUSE_AXIS_WHEEL;
    e.axis.x      = x;
    e.axis.y      = y;
    e.axis.z      = 0;
    e.axis.mods   = 0;
    walrus_event_push(&e);
}

Walrus_MouseButton translate_btn(i32 btn)
{
    switch (btn) {
        case GLFW_MOUSE_BUTTON_LEFT:
            return WR_MOUSE_BTN_LEFT;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return WR_MOUSE_BTN_RIGHT;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return WR_MOUSE_BTN_MIDDLE;
    }
    return WR_MOUSE_BTN_COUNT;
}

static void mousebtn_callback(GLFWwindow *window, i32 btn, i32 action, i32 mods)
{
    walrus_unused(window);

    Walrus_Event e;
    e.type          = WR_EVENT_TYPE_BUTTON;
    e.button.device = WR_INPUT_MOUSE;
    e.button.button = translate_btn(btn);
    e.button.state  = action != GLFW_RELEASE;
    e.button.mods   = translate_keymods(mods);
    walrus_event_push(&e);
}

static void text_callback(GLFWwindow *window, u32 unicode)
{
    walrus_unused(window);

    Walrus_Event e;
    e.type         = WR_EVENT_TYPE_TEXT;
    e.text.unicode = unicode;
    walrus_event_push(&e);
}

void glfw_create_window(Walrus_Window *window, char const *title, u32 width, u32 height, u32 flags)
{
    walrus_assert_msg(glfwInit(), "Cannot initialize glfw!");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, flags & WR_WINDOW_FLAG_RESIZABLE ? GLFW_TRUE : GLFW_FALSE);
    glfwSetErrorCallback(error_callback);

    window->handle = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window->handle != NULL) {
        glfwSetWindowUserPointer(window->handle, window);
        glfwSetWindowCloseCallback(window->handle, window_close_callback);
        glfwSetWindowSizeCallback(window->handle, window_size_callback);
        glfwSetFramebufferSizeCallback(window->handle, framebuffer_size_callback);
        glfwSetKeyCallback(window->handle, key_callback);
        glfwSetCursorPosCallback(window->handle, cursor_callback);
        glfwSetScrollCallback(window->handle, scroll_callback);
        glfwSetMouseButtonCallback(window->handle, mousebtn_callback);
        glfwSetCharCallback(window->handle, text_callback);
    }
}

void glfw_destroy_window(void *handle)
{
    glfwDestroyWindow(handle);
}
