#include <input.h>
#include <event.h>

u16 translate_key(i16 key, bool alter)
{
    switch (key) {
        case 9:
            return KEYBOARD_BACKSPACE;
        case 10:
            return KEYBOARD_TAB;
        case 13:
            return KEYBOARD_ENTER;
        case 16:
            return alter ? KEYBOARD_RIGHT_SHIFT : KEYBOARD_LEFT_SHIFT;
        case 17:
            return alter ? KEYBOARD_RIGHT_CTRL : KEYBOARD_LEFT_CTRL;
        case 18:
            return alter ? KEYBOARD_RIGHT_ALT : KEYBOARD_LEFT_ALT;
        case 19:
            return KEYBOARD_PAUSE;
        case 20:
            return KEYBOARD_CAPS_LOCK;
        case 27:
            return KEYBOARD_ESCAPE;
        case 32:
            return KEYBOARD_SPACE;
        case 33:
            return KEYBOARD_PAGE_UP;
        case 34:
            return KEYBOARD_PAGE_DOWN;
        case 35:
            return KEYBOARD_END;
        case 36:
            return KEYBOARD_HOME;
        case 37:
            return KEYBOARD_LEFT_ARROW;
        case 38:
            return KEYBOARD_UP_ARROW;
        case 39:
            return KEYBOARD_RIGHT_ARROW;
        case 40:
            return KEYBOARD_DOWN_ARROW;
        case 44:
            return KEYBOARD_PRINT_SCREEN;
        case 45:
            return KEYBOARD_INSERT;
        case 46:
            return KEYBOARD_DELETE;
        case 48:
            return KEYBOARD_D0;
        case 49:
            return KEYBOARD_D1;
        case 50:
            return KEYBOARD_D2;
        case 51:
            return KEYBOARD_D3;
        case 52:
            return KEYBOARD_D4;
        case 53:
            return KEYBOARD_D5;
        case 54:
            return KEYBOARD_D6;
        case 55:
            return KEYBOARD_D7;
        case 56:
            return KEYBOARD_D8;
        case 57:
            return KEYBOARD_D9;
        case 65:
            return KEYBOARD_A;
        case 66:
            return KEYBOARD_B;
        case 67:
            return KEYBOARD_C;
        case 68:
            return KEYBOARD_D;
        case 69:
            return KEYBOARD_E;
        case 70:
            return KEYBOARD_F;
        case 71:
            return KEYBOARD_G;
        case 72:
            return KEYBOARD_H;
        case 73:
            return KEYBOARD_I;
        case 74:
            return KEYBOARD_J;
        case 75:
            return KEYBOARD_K;
        case 76:
            return KEYBOARD_L;
        case 77:
            return KEYBOARD_M;
        case 78:
            return KEYBOARD_N;
        case 79:
            return KEYBOARD_O;
        case 80:
            return KEYBOARD_P;
        case 81:
            return KEYBOARD_Q;
        case 82:
            return KEYBOARD_R;
        case 83:
            return KEYBOARD_S;
        case 84:
            return KEYBOARD_T;
        case 85:
            return KEYBOARD_U;
        case 86:
            return KEYBOARD_V;
        case 87:
            return KEYBOARD_W;
        case 88:
            return KEYBOARD_X;
        case 89:
            return KEYBOARD_Y;
        case 90:
            return KEYBOARD_Z;
        case 91:
            return KEYBOARD_LEFT_SUPER;
        case 92:
            return KEYBOARD_RIGHT_SUPER;
        case 96:
            return KEYBOARD_KEYPAD0;
        case 97:
            return KEYBOARD_KEYPAD1;
        case 98:
            return KEYBOARD_KEYPAD2;
        case 99:
            return KEYBOARD_KEYPAD3;
        case 100:
            return KEYBOARD_KEYPAD4;
        case 101:
            return KEYBOARD_KEYPAD5;
        case 102:
            return KEYBOARD_KEYPAD6;
        case 103:
            return KEYBOARD_KEYPAD7;
        case 104:
            return KEYBOARD_KEYPAD8;
        case 105:
            return KEYBOARD_KEYPAD9;
        case 106:
            return KEYBOARD_KEYPAD_MULTIPLY;
        case 107:
            return KEYBOARD_KEYPAD_ADD;
        case 109:
            return KEYBOARD_KEYPAD_SUBTRACT;
        case 110:
            return KEYBOARD_KEYPAD_DECIMAL;
        case 111:
            return KEYBOARD_KEYPAD_DIVIDE;
        case 112:
            return KEYBOARD_F1;
        case 113:
            return KEYBOARD_F2;
        case 114:
            return KEYBOARD_F3;
        case 115:
            return KEYBOARD_F4;
        case 116:
            return KEYBOARD_F5;
        case 117:
            return KEYBOARD_F6;
        case 118:
            return KEYBOARD_F7;
        case 119:
            return KEYBOARD_F8;
        case 120:
            return KEYBOARD_F9;
        case 121:
            return KEYBOARD_F10;
        case 122:
            return KEYBOARD_F11;
        case 123:
            return KEYBOARD_F12;
        case 144:
            return KEYBOARD_NUM_LOCK;
        case 145:
            return KEYBOARD_SCROLL_LOCK;
        case 186:
            return KEYBOARD_SEMICOLON;
        case 187:
            return KEYBOARD_EQUAL;
        case 188:
            return KEYBOARD_COMMA;
        case 189:
            return KEYBOARD_MINUS;
        case 190:
            return KEYBOARD_PERIOD;
        case 191:
            return KEYBOARD_SLASH;
        case 192:
            return KEYBOARD_BACK_QUOTE;
        case 219:
            return KEYBOARD_LEFT_BRACKET;
        case 220:
            return KEYBOARD_BACK_SLASH;
        case 221:
            return KEYBOARD_RIGHT_BRACKET;
        case 222:
            return KEYBOARD_QUOTE;
    }
    return KEYBOARD_UNKNOWN;
}

void __on_mouse_move(i32 x, i32 y, u8 mods)
{
    Event e;
    e.type        = EVENT_TYPE_AXIS;
    e.axis.device = INPUT_MOUSE;
    e.axis.axis   = 0;
    e.axis.x      = x;
    e.axis.y      = y;
    e.axis.z      = 0;
    e.axis.mods   = mods;
    event_push(&e);
}

void __on_mouse_up(i8 btn, u8 mods)
{
    Event e;
    e.type          = EVENT_TYPE_BUTTON;
    e.button.device = INPUT_MOUSE;
    e.button.button = btn;
    e.button.state  = true;
    e.button.mods   = mods;
    event_push(&e);
}

void __on_mouse_down(i8 btn, u8 mods)
{
    Event e;
    e.type          = EVENT_TYPE_BUTTON;
    e.button.device = INPUT_MOUSE;
    e.button.button = btn;
    e.button.state  = false;
    e.button.mods   = mods;
    event_push(&e);
}

void __on_key_down(i16 key, u8 mods, i8 location)
{
    Event e;
    e.type          = EVENT_TYPE_BUTTON;
    e.button.device = INPUT_KEYBOARD;
    e.button.button = translate_key(key, location == 1);
    e.button.state  = true;
    e.button.mods   = mods;
    event_push(&e);
}

void __on_key_up(i16 key, u8 mods, i8 location)
{
    Event e;
    e.type          = EVENT_TYPE_BUTTON;
    e.button.device = INPUT_KEYBOARD;
    e.button.button = translate_key(key, location == 1);
    e.button.state  = false;
    e.button.mods   = mods;
    event_push(&e);
}

void __on_exit(void)
{
    Event e;
    e.type = EVENT_TYPE_EXIT;
    event_push(&e);
}
