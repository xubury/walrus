#include <engine/input.h>
#include <engine/event.h>

static u16 translate_key(i16 key, bool alter)
{
    switch (key) {
        case 9:
            return WR_KEY_BACKSPACE;
        case 10:
            return WR_KEY_TAB;
        case 13:
            return WR_KEY_ENTER;
        case 16:
            return alter ? WR_KEY_RIGHT_SHIFT : WR_KEY_LEFT_SHIFT;
        case 17:
            return alter ? WR_KEY_RIGHT_CTRL : WR_KEY_LEFT_CTRL;
        case 18:
            return alter ? WR_KEY_RIGHT_ALT : WR_KEY_LEFT_ALT;
        case 19:
            return WR_KEY_PAUSE;
        case 20:
            return WR_KEY_CAPS_LOCK;
        case 27:
            return WR_KEY_ESCAPE;
        case 32:
            return WR_KEY_SPACE;
        case 33:
            return WR_KEY_PAGE_UP;
        case 34:
            return WR_KEY_PAGE_DOWN;
        case 35:
            return WR_KEY_END;
        case 36:
            return WR_KEY_HOME;
        case 37:
            return WR_KEY_LEFT_ARROW;
        case 38:
            return WR_KEY_UP_ARROW;
        case 39:
            return WR_KEY_RIGHT_ARROW;
        case 40:
            return WR_KEY_DOWN_ARROW;
        case 44:
            return WR_KEY_PRINT_SCREEN;
        case 45:
            return WR_KEY_INSERT;
        case 46:
            return WR_KEY_DELETE;
        case 48:
            return WR_KEY_D0;
        case 49:
            return WR_KEY_D1;
        case 50:
            return WR_KEY_D2;
        case 51:
            return WR_KEY_D3;
        case 52:
            return WR_KEY_D4;
        case 53:
            return WR_KEY_D5;
        case 54:
            return WR_KEY_D6;
        case 55:
            return WR_KEY_D7;
        case 56:
            return WR_KEY_D8;
        case 57:
            return WR_KEY_D9;
        case 65:
            return WR_KEY_A;
        case 66:
            return WR_KEY_B;
        case 67:
            return WR_KEY_C;
        case 68:
            return WR_KEY_D;
        case 69:
            return WR_KEY_E;
        case 70:
            return WR_KEY_F;
        case 71:
            return WR_KEY_G;
        case 72:
            return WR_KEY_H;
        case 73:
            return WR_KEY_I;
        case 74:
            return WR_KEY_J;
        case 75:
            return WR_KEY_K;
        case 76:
            return WR_KEY_L;
        case 77:
            return WR_KEY_M;
        case 78:
            return WR_KEY_N;
        case 79:
            return WR_KEY_O;
        case 80:
            return WR_KEY_P;
        case 81:
            return WR_KEY_Q;
        case 82:
            return WR_KEY_R;
        case 83:
            return WR_KEY_S;
        case 84:
            return WR_KEY_T;
        case 85:
            return WR_KEY_U;
        case 86:
            return WR_KEY_V;
        case 87:
            return WR_KEY_W;
        case 88:
            return WR_KEY_X;
        case 89:
            return WR_KEY_Y;
        case 90:
            return WR_KEY_Z;
        case 91:
            return WR_KEY_LEFT_SUPER;
        case 92:
            return WR_KEY_RIGHT_SUPER;
        case 96:
            return WR_KEY_KEYPAD0;
        case 97:
            return WR_KEY_KEYPAD1;
        case 98:
            return WR_KEY_KEYPAD2;
        case 99:
            return WR_KEY_KEYPAD3;
        case 100:
            return WR_KEY_KEYPAD4;
        case 101:
            return WR_KEY_KEYPAD5;
        case 102:
            return WR_KEY_KEYPAD6;
        case 103:
            return WR_KEY_KEYPAD7;
        case 104:
            return WR_KEY_KEYPAD8;
        case 105:
            return WR_KEY_KEYPAD9;
        case 106:
            return WR_KEY_KEYPAD_MULTIPLY;
        case 107:
            return WR_KEY_KEYPAD_ADD;
        case 109:
            return WR_KEY_KEYPAD_SUBTRACT;
        case 110:
            return WR_KEY_KEYPAD_DECIMAL;
        case 111:
            return WR_KEY_KEYPAD_DIVIDE;
        case 112:
            return WR_KEY_F1;
        case 113:
            return WR_KEY_F2;
        case 114:
            return WR_KEY_F3;
        case 115:
            return WR_KEY_F4;
        case 116:
            return WR_KEY_F5;
        case 117:
            return WR_KEY_F6;
        case 118:
            return WR_KEY_F7;
        case 119:
            return WR_KEY_F8;
        case 120:
            return WR_KEY_F9;
        case 121:
            return WR_KEY_F10;
        case 122:
            return WR_KEY_F11;
        case 123:
            return WR_KEY_F12;
        case 144:
            return WR_KEY_NUM_LOCK;
        case 145:
            return WR_KEY_SCROLL_LOCK;
        case 186:
            return WR_KEY_SEMICOLON;
        case 187:
            return WR_KEY_EQUAL;
        case 188:
            return WR_KEY_COMMA;
        case 189:
            return WR_KEY_MINUS;
        case 190:
            return WR_KEY_PERIOD;
        case 191:
            return WR_KEY_SLASH;
        case 192:
            return WR_KEY_BACK_QUOTE;
        case 219:
            return WR_KEY_LEFT_BRACKET;
        case 220:
            return WR_KEY_BACK_SLASH;
        case 221:
            return WR_KEY_RIGHT_BRACKET;
        case 222:
            return WR_KEY_QUOTE;
    }
    return WR_KEY_UNKNOWN;
}

void __on_mouse_move(i32 x, i32 y, u8 mods)
{
    Walrus_Event e;
    e.type        = WR_EVENT_TYPE_AXIS;
    e.axis.device = WR_INPUT_MOUSE;
    e.axis.axis   = WR_MOUSE_AXIS_CURSOR;
    e.axis.x      = x;
    e.axis.y      = y;
    e.axis.z      = 0;
    e.axis.mods   = mods;
    walrus_event_push(&e);
}

void __on_mouse_scroll(i32 x_offset, i32 y_offset)
{
    Walrus_Event e;
    static f64   x = 0;
    static f64   y = 0;
    x += x_offset;
    y += y_offset;
    e.type        = WR_EVENT_TYPE_AXIS;
    e.axis.device = WR_INPUT_MOUSE;
    e.axis.axis   = WR_MOUSE_AXIS_WHEEL;
    e.axis.x      = x;
    e.axis.y      = y;
    e.axis.z      = 0;
    walrus_event_push(&e);
}

void __on_mouse_up(i8 btn, u8 mods)
{
    Walrus_Event e;
    e.type          = WR_EVENT_TYPE_BUTTON;
    e.button.device = WR_INPUT_MOUSE;
    e.button.button = btn;
    e.button.state  = false;
    e.button.mods   = mods;
    walrus_event_push(&e);
}

void __on_mouse_down(i8 btn, u8 mods)
{
    Walrus_Event e;
    e.type          = WR_EVENT_TYPE_BUTTON;
    e.button.device = WR_INPUT_MOUSE;
    e.button.button = btn;
    e.button.state  = true;
    e.button.mods   = mods;
    walrus_event_push(&e);
}

void __on_key_down(i16 key, u8 mods, i8 location)
{
    Walrus_Event e;
    e.type          = WR_EVENT_TYPE_BUTTON;
    e.button.device = WR_INPUT_KEYBOARD;
    e.button.button = translate_key(key, location == 1);
    e.button.state  = true;
    e.button.mods   = mods;
    walrus_event_push(&e);
}

void __on_key_up(i16 key, u8 mods, i8 location)
{
    Walrus_Event e;
    e.type          = WR_EVENT_TYPE_BUTTON;
    e.button.device = WR_INPUT_KEYBOARD;
    e.button.button = translate_key(key, location == 1);
    e.button.state  = false;
    e.button.mods   = mods;
    walrus_event_push(&e);
}

void __on_exit(void)
{
    Walrus_Event e;
    e.type = WR_EVENT_TYPE_EXIT;
    walrus_event_push(&e);
}
