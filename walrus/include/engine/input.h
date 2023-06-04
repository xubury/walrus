#pragma once

#include <engine/input_device.h>

typedef enum {
    WR_MOUSE_BTN_LEFT   = 0,
    WR_MOUSE_BTN_MIDDLE = 1,
    WR_MOUSE_BTN_RIGHT  = 2,

    WR_MOUSE_BTN_COUNT,
} Walrus_MouseButton;

typedef enum {
    WR_MOUSE_AXIS_CURSOR = 0,
    WR_MOUSE_AXIS_WHEEL = 1,

    WR_MOUSE_AXIS_COUNT,
} Walrus_MouseAxis;

typedef enum {
    WR_KEYMOD_NONE  = 0,
    WR_KEYMOD_ALT   = 1 << 0,
    WR_KEYMOD_CTRL  = 1 << 1,
    WR_KEYMOD_SHIFT = 1 << 2,
    WR_KEYMOD_SUPER = 1 << 3,
} Walrus_Keymod;

typedef enum {
    WR_KEY_SPACE = 0,
    WR_KEY_QUOTE,  /* ' */
    WR_KEY_COMMA,  /* , */
    WR_KEY_MINUS,  /* - */
    WR_KEY_PERIOD, /* . */
    WR_KEY_SLASH,  /* / */

    WR_KEY_D0, /* 0 */
    WR_KEY_D1, /* 1 */
    WR_KEY_D2, /* 2 */
    WR_KEY_D3, /* 3 */
    WR_KEY_D4, /* 4 */
    WR_KEY_D5, /* 5 */
    WR_KEY_D6, /* 6 */
    WR_KEY_D7, /* 7 */
    WR_KEY_D8, /* 8 */
    WR_KEY_D9, /* 9 */

    WR_KEY_SEMICOLON, /* ; */
    WR_KEY_EQUAL,     /* = */

    WR_KEY_A,
    WR_KEY_B,
    WR_KEY_C,
    WR_KEY_D,
    WR_KEY_E,
    WR_KEY_F,
    WR_KEY_G,
    WR_KEY_H,
    WR_KEY_I,
    WR_KEY_J,
    WR_KEY_K,
    WR_KEY_L,
    WR_KEY_M,
    WR_KEY_N,
    WR_KEY_O,
    WR_KEY_P,
    WR_KEY_Q,
    WR_KEY_R,
    WR_KEY_S,
    WR_KEY_T,
    WR_KEY_U,
    WR_KEY_V,
    WR_KEY_W,
    WR_KEY_X,
    WR_KEY_Y,
    WR_KEY_Z,

    WR_KEY_LEFT_BRACKET,  /* [ */
    WR_KEY_BACK_SLASH,    /* \ */
    WR_KEY_RIGHT_BRACKET, /* ] */
    WR_KEY_BACK_QUOTE,    /* ` */

    WR_KEY_WORLD1, /* NON-US #1 */
    WR_KEY_WORLD2, /* NON-US #2 */

    /* FUNCTION KEYS */
    WR_KEY_ESCAPE,
    WR_KEY_ENTER,
    WR_KEY_TAB,
    WR_KEY_BACKSPACE,
    WR_KEY_INSERT,
    WR_KEY_DELETE,
    WR_KEY_RIGHT_ARROW,
    WR_KEY_LEFT_ARROW,
    WR_KEY_DOWN_ARROW,
    WR_KEY_UP_ARROW,
    WR_KEY_PAGE_UP,
    WR_KEY_PAGE_DOWN,
    WR_KEY_HOME,
    WR_KEY_END,
    WR_KEY_CAPS_LOCK,
    WR_KEY_SCROLL_LOCK,
    WR_KEY_NUM_LOCK,
    WR_KEY_PRINT_SCREEN,
    WR_KEY_PAUSE,
    WR_KEY_F1,
    WR_KEY_F2,
    WR_KEY_F3,
    WR_KEY_F4,
    WR_KEY_F5,
    WR_KEY_F6,
    WR_KEY_F7,
    WR_KEY_F8,
    WR_KEY_F9,
    WR_KEY_F10,
    WR_KEY_F11,
    WR_KEY_F12,
    WR_KEY_F13,
    WR_KEY_F14,
    WR_KEY_F15,
    WR_KEY_F16,
    WR_KEY_F17,
    WR_KEY_F18,
    WR_KEY_F19,
    WR_KEY_F20,
    WR_KEY_F21,
    WR_KEY_F22,
    WR_KEY_F23,
    WR_KEY_F24,
    WR_KEY_F25,

    /* KEYPAD */
    WR_KEY_KEYPAD0,
    WR_KEY_KEYPAD1,
    WR_KEY_KEYPAD2,
    WR_KEY_KEYPAD3,
    WR_KEY_KEYPAD4,
    WR_KEY_KEYPAD5,
    WR_KEY_KEYPAD6,
    WR_KEY_KEYPAD7,
    WR_KEY_KEYPAD8,
    WR_KEY_KEYPAD9,
    WR_KEY_KEYPAD_DECIMAL,
    WR_KEY_KEYPAD_DIVIDE,
    WR_KEY_KEYPAD_MULTIPLY,
    WR_KEY_KEYPAD_SUBTRACT,
    WR_KEY_KEYPAD_ADD,
    WR_KEY_KEYPAD_ENTER,
    WR_KEY_KEYPAD_EQUAL,

    WR_KEY_LEFT_SHIFT,
    WR_KEY_LEFT_CTRL,
    WR_KEY_LEFT_ALT,
    WR_KEY_LEFT_SUPER,
    WR_KEY_RIGHT_SHIFT,
    WR_KEY_RIGHT_CTRL,
    WR_KEY_RIGHT_ALT,
    WR_KEY_RIGHT_SUPER,
    WR_KEY_MENU,

    WR_KEY_COUNT,
    WR_KEY_UNKNOWN = 0XFF,

} Walrus_Keyboard;

typedef struct {
    Walrus_InputDevice *mouse;
    Walrus_InputDevice *keyboard;
} Walrus_Input;

Walrus_Input *walrus_inputs_create(void);

void walrus_inputs_destroy(Walrus_Input *input);

void walrus_inputs_tick(Walrus_Input *input);
