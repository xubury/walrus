#include <engine/input.h>
#include <core/memory.h>

Walrus_Input *walrus_inputs_create(void)
{
    Walrus_Input *input = walrus_malloc(sizeof(Walrus_Input));
    input->mouse        = walrus_input_create(WR_MOUSE_BTN_COUNT, WR_MOUSE_AXIS_COUNT);
    input->keyboard     = walrus_input_create(WR_KEY_COUNT, 0);

    return input;
}

void walrus_inputs_destroy(Walrus_Input *input)
{
    walrus_input_destroy(input->mouse);
    walrus_input_destroy(input->keyboard);
    walrus_free(input);
}

void walrus_inputs_tick(Walrus_Input *input)
{
    walrus_input_tick(input->mouse);
    walrus_input_tick(input->keyboard);
}
