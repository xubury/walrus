#pragma once

#include <input_device.h>

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
    KEYCODE_A = 65,

    KEYCODE_COUNT
} Keycode;

typedef struct {
    InputDevice *mouse;
    InputDevice *keyboard;
} Input;

Input *inputs_create(void);

void inputs_destroy(Input *input);

void inputs_tick(Input *input);
