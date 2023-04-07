#include <engine/input_device.h>
#include <core/macro.h>

#include <string.h>

struct _InputDevice {
    u16 num_buttons;
    u16 first_buttons[2];

    bool* state;
    bool* last_state;

    u8 modifiers;
    u8 num_axes;

    vec3* axis;
    vec3* last_axis;
};

Walrus_InputDevice* walrus_input_create(i16 num_btns, i8 num_axes)
{
    Walrus_InputDevice* device = malloc(sizeof(Walrus_InputDevice));

    if (device) {
        device->num_buttons = num_btns;
        device->state       = malloc(num_btns * sizeof(bool));
        device->last_state  = malloc(num_btns * sizeof(bool));
        device->modifiers   = 0;
        device->num_axes    = num_axes;
        device->axis        = malloc(num_axes * sizeof(vec3));
        device->last_axis   = malloc(num_axes * sizeof(vec3));

        memset(device->first_buttons, 0, walrus_array_len(device->first_buttons) * sizeof(u16));
        memset(device->state, 0, num_btns * sizeof(bool));
        memset(device->last_state, 0, num_btns * sizeof(bool));
        memset(device->axis, 0, num_axes * sizeof(vec3));
        memset(device->last_axis, 0, num_axes * sizeof(vec3));
    }

    return device;
}

void walrus_input_destroy(Walrus_InputDevice* device)
{
    free(device->state);
    free(device->last_state);
    free(device->axis);
    free(device->last_axis);
    free(device);
}

bool walrus_input_pressed(Walrus_InputDevice* device, i16 id)
{
    return id < device->num_buttons ? (!device->last_state[id] & device->state[id]) : false;
}

u16 walrus_input_any_pressed(Walrus_InputDevice* device)
{
    return walrus_input_pressed(device, device->first_buttons[0]) ? device->first_buttons[0] : WR_INPUT_INVALID_ID;
}

bool walrus_input_released(Walrus_InputDevice* device, i16 id)
{
    return id < device->num_buttons ? (device->last_state[id] & !device->state[id]) : false;
}

u16 walrus_input_any_released(Walrus_InputDevice* device)
{
    return walrus_input_released(device, device->first_buttons[1]) ? device->first_buttons[1] : WR_INPUT_INVALID_ID;
}

bool walrus_input_down(Walrus_InputDevice* device, i16 id)
{
    return id < device->num_buttons ? device->state[id] : false;
}

u16 walrus_input_any_down(Walrus_InputDevice* device)
{
    return walrus_input_down(device, device->first_buttons[0]) ? device->first_buttons[0] : WR_INPUT_INVALID_ID;
}

void walrus_input_set_button(Walrus_InputDevice* device, i16 id, bool state, u8 modifiers)
{
    device->state[id]                    = state;
    device->first_buttons[state ? 0 : 1] = id;
    device->modifiers                    = modifiers;
}

u8 walrus_input_modifiers(Walrus_InputDevice* device)
{
    return device->modifiers;
}

void walrus_input_axis(Walrus_InputDevice* device, i8 id, vec3 out)
{
    id < device->num_axes ? glm_vec3_copy(out, device->axis[id]) : glm_vec3_copy(out, (vec3){0, 0, 0});
}

void walrus_input_relaxis(Walrus_InputDevice* device, i8 id, vec3 out)
{
    id < device->num_axes ? glm_vec3_sub(device->axis[id], device->last_axis[id], out)
                          : glm_vec3_copy(out, (vec3){0, 0, 0});
}

void walrus_input_set_axis(Walrus_InputDevice* device, i8 id, f32 x, f32 y, f32 z, u8 modifiers)
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
