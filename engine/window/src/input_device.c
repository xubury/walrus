#include <input_device.h>

struct _InputDevice {
    u16 num_buttons;
    u16 first_buttons[2];

    bool* state;
    bool* last_state;

    u8 modifiers;
    u8 numAxes;

    vec3* axis;
    vec3* lastAxis;
};

bool pressed(InputDevice* device, u16 id)
{
    return id < device->num_buttons ? (!device->last_state[id] & device->state[id]) : false;
}

u16 any_pressed(InputDevice* device)
{
    return pressed(device, device->first_buttons[0]) ? device->first_buttons[0] : INTPUT_INVALID_ID;
}
