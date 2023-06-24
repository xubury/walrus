#include <engine/input_device.h>
#include <core/macro.h>
#include <core/memory.h>
#include <core/math.h>

#include <string.h>

struct Walrus_InputDevice {
    u16 num_buttons;
    u16 first_buttons[2];

    bool* state;
    bool* last_state;

    u8 modifiers;
    u8 num_axes;

    vec3* axis;
    vec3* last_axis;
};

Walrus_InputDevice* walrus_input_create(u16 num_btns, u8 num_axes)
{
    Walrus_InputDevice* device = walrus_malloc(sizeof(Walrus_InputDevice));

    if (device) {
        device->num_buttons = num_btns;
        device->state       = walrus_malloc0(num_btns * sizeof(bool));
        device->last_state  = walrus_malloc0(num_btns * sizeof(bool));
        device->modifiers   = 0;
        device->num_axes    = num_axes;
        device->axis        = walrus_malloc0(num_axes * sizeof(vec3));
        device->last_axis   = walrus_malloc0(num_axes * sizeof(vec3));
    }

    return device;
}

void walrus_input_destroy(Walrus_InputDevice* device)
{
    walrus_free(device->state);
    walrus_free(device->last_state);
    walrus_free(device->axis);
    walrus_free(device->last_axis);
    walrus_free(device);
}

bool walrus_input_pressed(Walrus_InputDevice* device, u16 id)
{
    return id < device->num_buttons ? (!device->last_state[id] & device->state[id]) : false;
}

bool walrus_input_any_pressed(Walrus_InputDevice* device)
{
    return walrus_input_pressed(device, device->first_buttons[0]);
}

bool walrus_input_released(Walrus_InputDevice* device, u16 id)
{
    return id < device->num_buttons ? (device->last_state[id] & !device->state[id]) : false;
}

bool walrus_input_any_released(Walrus_InputDevice* device)
{
    return walrus_input_released(device, device->first_buttons[1]);
}

bool walrus_input_down(Walrus_InputDevice* device, u16 id)
{
    return id < device->num_buttons ? device->state[id] : false;
}

bool walrus_input_any_down(Walrus_InputDevice* device)
{
    return walrus_input_down(device, device->first_buttons[0]);
}

void walrus_input_set_button(Walrus_InputDevice* device, u16 id, bool state, u8 modifiers)
{
    device->state[id]                    = state;
    device->first_buttons[state ? 0 : 1] = id;
    device->modifiers                    = modifiers;
}

u8 walrus_input_modifiers(Walrus_InputDevice* device)
{
    return device->modifiers;
}

void walrus_input_axis(Walrus_InputDevice* device, u8 id, f32* x, f32* y, f32* z)
{
    bool const valid = id < device->num_axes;
    if (x) *x = valid ? device->axis[id][0] : 0;
    if (y) *y = valid ? device->axis[id][1] : 0;
    if (z) *z = valid ? device->axis[id][2] : 0;
}

void walrus_input_relaxis(Walrus_InputDevice* device, u8 id, f32* x, f32* y, f32* z)
{
    bool const valid = id < device->num_axes;
    if (x) *x = valid ? device->axis[id][0] - device->last_axis[id][0] : 0;
    if (y) *y = valid ? device->axis[id][1] - device->last_axis[id][1] : 0;
    if (z) *z = valid ? device->axis[id][2] - device->last_axis[id][2] : 0;
}

void walrus_input_set_axis(Walrus_InputDevice* device, u8 id, f32 x, f32 y, f32 z, u8 modifiers)
{
    device->axis[id][0] = x;
    device->axis[id][1] = y;
    device->axis[id][2] = z;
    device->modifiers   = modifiers;
}

void walrus_input_tick(Walrus_InputDevice* device)
{
    memcpy(device->last_state, device->state, device->num_buttons * sizeof(bool));
    memcpy(device->last_axis, device->axis, device->num_axes * sizeof(vec3));
}
