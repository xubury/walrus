#include <input.h>

Input *inputs_create(void)
{
    Input *input    = malloc(sizeof(Input));
    input->mouse    = input_create(MOUSE_BTN_COUNT, 1);
    input->keyboard = input_create(KEYBOARD_COUNT, 0);

    return input;
}

void inputs_destroy(Input *input)
{
    input_destroy(input->mouse);
    input_destroy(input->keyboard);
    free(input);
}

void inputs_tick(Input *input)
{
    input_tick(input->mouse);
    input_tick(input->keyboard);
}
