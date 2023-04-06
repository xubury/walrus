#include <glfw_window.h>
#include <window.h>

#include <macro.h>
#include <event.h>
#include <input.h>
#include <log.h>

#include <GLFW/glfw3.h>

static void error_callback(i32 error, char const *description)
{
    log_error("GLFW Error ({%d}): {%s}\n", error, description);
}

static void framebuffer_size_callback(GLFWwindow *window, i32 width, i32 height)
{
    UNUSED(window);
    Event e;
    e.type              = EVENT_TYPE_RESOLUTION;
    e.resolution.width  = width;
    e.resolution.height = height;
    event_push(&e);
}

static void window_close_callback(GLFWwindow *window)
{
    UNUSED(window);
    Event e;
    e.type = EVENT_TYPE_EXIT;
    event_push(&e);
}

static u16 translate_key(i32 keycode)
{
    switch (keycode) {
        case GLFW_KEY_TAB:
            return KEYBOARD_TAB;
        case GLFW_KEY_LEFT:
            return KEYBOARD_LEFT_ARROW;
        case GLFW_KEY_RIGHT:
            return KEYBOARD_RIGHT_ARROW;
        case GLFW_KEY_UP:
            return KEYBOARD_UP_ARROW;
        case GLFW_KEY_DOWN:
            return KEYBOARD_DOWN_ARROW;
        case GLFW_KEY_PAGE_UP:
            return KEYBOARD_PAGE_UP;
        case GLFW_KEY_PAGE_DOWN:
            return KEYBOARD_PAGE_DOWN;
        case GLFW_KEY_HOME:
            return KEYBOARD_HOME;
        case GLFW_KEY_END:
            return KEYBOARD_END;
        case GLFW_KEY_INSERT:
            return KEYBOARD_INSERT;
        case GLFW_KEY_DELETE:
            return KEYBOARD_DELETE;
        case GLFW_KEY_BACKSPACE:
            return KEYBOARD_BACKSPACE;
        case GLFW_KEY_SPACE:
            return KEYBOARD_SPACE;
        case GLFW_KEY_ENTER:
            return KEYBOARD_ENTER;
        case GLFW_KEY_ESCAPE:
            return KEYBOARD_ESCAPE;
        case GLFW_KEY_APOSTROPHE:
            return KEYBOARD_QUOTE;
        case GLFW_KEY_COMMA:
            return KEYBOARD_COMMA;
        case GLFW_KEY_MINUS:
            return KEYBOARD_MINUS;
        case GLFW_KEY_PERIOD:
            return KEYBOARD_PERIOD;
        case GLFW_KEY_SLASH:
            return KEYBOARD_SLASH;
        case GLFW_KEY_SEMICOLON:
            return KEYBOARD_SEMICOLON;
        case GLFW_KEY_EQUAL:
            return KEYBOARD_EQUAL;
        case GLFW_KEY_LEFT_BRACKET:
            return KEYBOARD_LEFT_BRACKET;
        case GLFW_KEY_BACKSLASH:
            return KEYBOARD_BACK_SLASH;
        case GLFW_KEY_RIGHT_BRACKET:
            return KEYBOARD_RIGHT_BRACKET;
        case GLFW_KEY_GRAVE_ACCENT:
            return KEYBOARD_BACK_QUOTE;
        case GLFW_KEY_CAPS_LOCK:
            return KEYBOARD_CAPS_LOCK;
        case GLFW_KEY_SCROLL_LOCK:
            return KEYBOARD_SCROLL_LOCK;
        case GLFW_KEY_NUM_LOCK:
            return KEYBOARD_NUM_LOCK;
        case GLFW_KEY_PRINT_SCREEN:
            return KEYBOARD_PRINT_SCREEN;
        case GLFW_KEY_PAUSE:
            return KEYBOARD_PAUSE;
        case GLFW_KEY_KP_0:
            return KEYBOARD_KEYPAD0;
        case GLFW_KEY_KP_1:
            return KEYBOARD_KEYPAD1;
        case GLFW_KEY_KP_2:
            return KEYBOARD_KEYPAD2;
        case GLFW_KEY_KP_3:
            return KEYBOARD_KEYPAD3;
        case GLFW_KEY_KP_4:
            return KEYBOARD_KEYPAD4;
        case GLFW_KEY_KP_5:
            return KEYBOARD_KEYPAD5;
        case GLFW_KEY_KP_6:
            return KEYBOARD_KEYPAD6;
        case GLFW_KEY_KP_7:
            return KEYBOARD_KEYPAD7;
        case GLFW_KEY_KP_8:
            return KEYBOARD_KEYPAD8;
        case GLFW_KEY_KP_9:
            return KEYBOARD_KEYPAD9;
        case GLFW_KEY_KP_DECIMAL:
            return KEYBOARD_KEYPAD_DECIMAL;
        case GLFW_KEY_KP_DIVIDE:
            return KEYBOARD_KEYPAD_DIVIDE;
        case GLFW_KEY_KP_MULTIPLY:
            return KEYBOARD_KEYPAD_MULTIPLY;
        case GLFW_KEY_KP_SUBTRACT:
            return KEYBOARD_KEYPAD_SUBTRACT;
        case GLFW_KEY_KP_ADD:
            return KEYBOARD_KEYPAD_ADD;
        case GLFW_KEY_KP_ENTER:
            return KEYBOARD_KEYPAD_ENTER;
        case GLFW_KEY_KP_EQUAL:
            return KEYBOARD_KEYPAD_EQUAL;
        case GLFW_KEY_LEFT_SHIFT:
            return KEYBOARD_LEFT_SHIFT;
        case GLFW_KEY_LEFT_CONTROL:
            return KEYBOARD_LEFT_CTRL;
        case GLFW_KEY_LEFT_ALT:
            return KEYBOARD_LEFT_ALT;
        case GLFW_KEY_LEFT_SUPER:
            return KEYBOARD_LEFT_SUPER;
        case GLFW_KEY_RIGHT_SHIFT:
            return KEYBOARD_RIGHT_SHIFT;
        case GLFW_KEY_RIGHT_CONTROL:
            return KEYBOARD_RIGHT_CTRL;
        case GLFW_KEY_RIGHT_ALT:
            return KEYBOARD_RIGHT_ALT;
        case GLFW_KEY_RIGHT_SUPER:
            return KEYBOARD_RIGHT_SUPER;
        case GLFW_KEY_MENU:
            return KEYBOARD_MENU;
        case GLFW_KEY_0:
            return KEYBOARD_D0;
        case GLFW_KEY_1:
            return KEYBOARD_D1;
        case GLFW_KEY_2:
            return KEYBOARD_D2;
        case GLFW_KEY_3:
            return KEYBOARD_D3;
        case GLFW_KEY_4:
            return KEYBOARD_D4;
        case GLFW_KEY_5:
            return KEYBOARD_D5;
        case GLFW_KEY_6:
            return KEYBOARD_D6;
        case GLFW_KEY_7:
            return KEYBOARD_D7;
        case GLFW_KEY_8:
            return KEYBOARD_D8;
        case GLFW_KEY_9:
            return KEYBOARD_D9;
        case GLFW_KEY_A:
            return KEYBOARD_A;
        case GLFW_KEY_B:
            return KEYBOARD_B;
        case GLFW_KEY_C:
            return KEYBOARD_C;
        case GLFW_KEY_D:
            return KEYBOARD_D;
        case GLFW_KEY_E:
            return KEYBOARD_E;
        case GLFW_KEY_F:
            return KEYBOARD_F;
        case GLFW_KEY_G:
            return KEYBOARD_G;
        case GLFW_KEY_H:
            return KEYBOARD_H;
        case GLFW_KEY_I:
            return KEYBOARD_I;
        case GLFW_KEY_J:
            return KEYBOARD_J;
        case GLFW_KEY_K:
            return KEYBOARD_K;
        case GLFW_KEY_L:
            return KEYBOARD_L;
        case GLFW_KEY_M:
            return KEYBOARD_M;
        case GLFW_KEY_N:
            return KEYBOARD_N;
        case GLFW_KEY_O:
            return KEYBOARD_O;
        case GLFW_KEY_P:
            return KEYBOARD_P;
        case GLFW_KEY_Q:
            return KEYBOARD_Q;
        case GLFW_KEY_R:
            return KEYBOARD_R;
        case GLFW_KEY_S:
            return KEYBOARD_S;
        case GLFW_KEY_T:
            return KEYBOARD_T;
        case GLFW_KEY_U:
            return KEYBOARD_U;
        case GLFW_KEY_V:
            return KEYBOARD_V;
        case GLFW_KEY_W:
            return KEYBOARD_W;
        case GLFW_KEY_X:
            return KEYBOARD_X;
        case GLFW_KEY_Y:
            return KEYBOARD_Y;
        case GLFW_KEY_Z:
            return KEYBOARD_Z;
        case GLFW_KEY_F1:
            return KEYBOARD_F1;
        case GLFW_KEY_F2:
            return KEYBOARD_F2;
        case GLFW_KEY_F3:
            return KEYBOARD_F3;
        case GLFW_KEY_F4:
            return KEYBOARD_F4;
        case GLFW_KEY_F5:
            return KEYBOARD_F5;
        case GLFW_KEY_F6:
            return KEYBOARD_F6;
        case GLFW_KEY_F7:
            return KEYBOARD_F7;
        case GLFW_KEY_F8:
            return KEYBOARD_F8;
        case GLFW_KEY_F9:
            return KEYBOARD_F9;
        case GLFW_KEY_F10:
            return KEYBOARD_F10;
        case GLFW_KEY_F11:
            return KEYBOARD_F11;
        case GLFW_KEY_F12:
            return KEYBOARD_F12;
        default:
            return KEYBOARD_UNKNOWN;
    }
}

static void key_callback(GLFWwindow *window, i32 keycode, i32 scancode, i32 action, i32 mods)
{
    UNUSED(window);
    UNUSED(scancode);

    Keyboard button = translate_key(keycode);
    if (button == KEYBOARD_UNKNOWN) {
        log_error("Unknown keycode: %d", keycode);
        return;
    }
    else {
        Event e;
        e.type          = EVENT_TYPE_BUTTON;
        e.button.button = button;
        e.button.device = INPUT_KEYBOARD;
        e.button.state  = action != GLFW_RELEASE;
        e.button.mods   = mods;
        event_push(&e);
    }
}

static void cursor_callback(GLFWwindow *window, f64 x, f64 y)
{
    UNUSED(window);

    Event e;
    e.type        = EVENT_TYPE_AXIS;
    e.axis.device = INPUT_MOUSE;
    e.axis.axis   = 0;
    e.axis.x      = x;
    e.axis.y      = y;
    e.axis.z      = 0;
    e.axis.mods   = 0;
    event_push(&e);
}

static void mousebtn_callback(GLFWwindow *window, i32 btn, i32 action, i32 mods)
{
    UNUSED(window);

    Event e;
    e.type          = EVENT_TYPE_BUTTON;
    e.button.device = INPUT_MOUSE;
    e.button.button = btn;
    e.button.state  = action != GLFW_RELEASE;
    e.button.mods   = mods;
}

void *glfw_create_window(char const *title, i32 width, i32 height, i32 flags)
{
    ASSERT_MSG(glfwInit(), "Cannot initialize glfw!");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSetErrorCallback(error_callback);

    void *handle = glfwCreateWindow(width, height, title, NULL, NULL);
    if (handle != NULL) {
        glfwMakeContextCurrent(handle);

        if (flags & WINDOW_FLAG_ASYNC) {
            glfwSwapInterval(1);
        }
        glfwSetWindowCloseCallback(handle, window_close_callback);
        glfwSetFramebufferSizeCallback(handle, framebuffer_size_callback);
        glfwSetKeyCallback(handle, key_callback);
        glfwSetCursorPosCallback(handle, cursor_callback);
        glfwSetMouseButtonCallback(handle, mousebtn_callback);
    }
    return handle;
}

void glfw_destroy_window(void *handle)
{
    glfwDestroyWindow(handle);
}
