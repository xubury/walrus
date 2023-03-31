#include <input.h>
#include <event.h>

#include <stdio.h>

void __on_mouse_move(i32 x, i32 y, u8 mods)
{
    Event e;
    e.type = EVENT_AXIS;
    e.axis.device = INPUT_MOUSE;
    e.axis.x = x;
    e.axis.y = y;
    e.axis.z = 0;
    e.axis.mods = mods;
    push_event(&e);
}

void __on_mouse_up(u8 btn, u8 mods)
{
    Event e;
    e.type = EVENT_BUTTON;
    e.button.device = INPUT_MOUSE;
    e.button.code = btn;
    e.button.state = true;
    e.button.mods = mods;
    push_event(&e);
}

void __on_mouse_down(u8 btn, u8 mods)
{
    Event e;
    e.type = EVENT_BUTTON;
    e.button.device = INPUT_MOUSE;
    e.button.code = btn;
    e.button.state = false;
    e.button.mods = mods;
    push_event(&e);
}

void __on_key_down(u16 btn, u8 mods)
{
    Event e;
    e.type = EVENT_BUTTON;
    e.button.device = INPUT_KEYBOARD;
    e.button.code = btn;
    e.button.state = true;
    e.button.mods = mods;
    push_event(&e);
}

void __on_key_up(u16 btn, u8 mods)
{
    Event e;
    e.type = EVENT_BUTTON;
    e.button.device = INPUT_KEYBOARD;
    e.button.code = btn;
    e.button.state = false;
    e.button.mods = mods;
    push_event(&e);
}
