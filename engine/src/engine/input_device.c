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

InputDevice* input_create(i16 num_btns, i8 num_axes)
{
    InputDevice* device = malloc(sizeof(InputDevice));

    if (device) {
        device->num_buttons = num_btns;
        device->state       = malloc(num_btns * sizeof(bool));
        device->last_state  = malloc(num_btns * sizeof(bool));
        device->modifiers   = 0;
        device->num_axes    = num_axes;
        device->axis        = malloc(num_axes * sizeof(vec3));
        device->last_axis   = malloc(num_axes * sizeof(vec3));

        memset(device->first_buttons, 0, ARRAY_LEN(device->first_buttons) * sizeof(u16));
        memset(device->state, 0, num_btns * sizeof(bool));
        memset(device->last_state, 0, num_btns * sizeof(bool));
        memset(device->axis, 0, num_axes * sizeof(vec3));
        memset(device->last_axis, 0, num_axes * sizeof(vec3));
    }

    return device;
}

void input_destroy(InputDevice* device)
{
    free(device->state);
    free(device->last_state);
    free(device->axis);
    free(device->last_axis);
    free(device);
}

bool input_pressed(InputDevice* device, i16 id)
{
    return id < device->num_buttons ? (!device->last_state[id] & device->state[id]) : false;
}

u16 input_any_pressed(InputDevice* device)
{
    return input_pressed(device, device->first_buttons[0]) ? device->first_buttons[0] : INTPUT_INVALID_ID;
}

bool input_released(InputDevice* device, i16 id)
{
    return id < device->num_buttons ? (device->last_state[id] & !device->state[id]) : false;
}

u16 input_any_released(InputDevice* device)
{
    return input_released(device, device->first_buttons[1]) ? device->first_buttons[1] : INTPUT_INVALID_ID;
}

bool input_down(InputDevice* device, i16 id)
{
    return id < device->num_buttons ? device->state[id] : false;
}

u16 input_any_down(InputDevice* device)
{
    return input_down(device, device->first_buttons[0]) ? device->first_buttons[0] : INTPUT_INVALID_ID;
}

void input_set_button(InputDevice* device, i16 id, bool state, u8 modifiers)
{
    device->state[id]                    = state;
    device->first_buttons[state ? 0 : 1] = id;
    device->modifiers                    = modifiers;
}

u8 input_modifiers(InputDevice* device)
{
    return device->modifiers;
}

void input_axis(InputDevice* device, i8 id, vec3 out)
{
    id < device->num_axes ? glm_vec3_copy(out, device->axis[id]) : glm_vec3_copy(out, (vec3){0, 0, 0});
}

void input_relaxis(InputDevice* device, i8 id, vec3 out)
{
    id < device->num_axes ? glm_vec3_sub(device->axis[id], device->last_axis[id], out)
                          : glm_vec3_copy(out, (vec3){0, 0, 0});
}

void input_set_axis(InputDevice* device, i8 id, f32 x, f32 y, f32 z, u8 modifiers)
{
    device->axis[id][0] = x;
    device->axis[id][1] = y;
    device->axis[id][2] = z;
    device->modifiers   = modifiers;
}

void input_tick(InputDevice* device)
{
    memcpy(device->last_state, device->state, device->num_buttons * sizeof(bool));
    memcpy(device->last_axis, device->axis, device->num_axes * sizeof(vec3));
}
