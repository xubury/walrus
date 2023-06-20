#include <engine/input.h>
#include <core/memory.h>

bool walrus_inputs_init(Walrus_Input *input)
{
    input->mouse    = walrus_input_create(WR_MOUSE_BTN_COUNT, WR_MOUSE_AXIS_COUNT);
    input->keyboard = walrus_input_create(WR_KEY_COUNT, 0);
    return input->mouse && input->keyboard;
}

void walrus_inputs_shutdown(Walrus_Input *input)
{
    walrus_input_destroy(input->mouse);
    walrus_input_destroy(input->keyboard);
}

void walrus_inputs_tick(Walrus_Input *input)
{
    walrus_input_tick(input->mouse);
    walrus_input_tick(input->keyboard);
}
