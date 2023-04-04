#pragma once

typedef enum {
    MOUSE_BTN_LEFT   = 0,
    MOUSE_BTN_MIDDLE = 1,
    MOUSE_BTN_RIGHT  = 2,

    MOUSE_BTN_COUNT,
} MouseButton;

typedef enum {
    KEYMOD_NONE  = 0,
    KEYMOD_ALT   = 1 << 1,
    KEYMOD_CTRL  = 1 << 2,
    KEYMOD_SHIFT = 1 << 3,
} KeyMod;

typedef enum {
    KEYCODE_A = 65
} Keycode;
