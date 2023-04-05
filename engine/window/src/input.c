#include <input.h>

Input *input_create(void)
{
    Input *input    = malloc(sizeof(Input));
    input->mouse    = input_device_create(MOUSE_BTN_COUNT, 2);
    input->keyboard = input_device_create(KEYCODE_COUNT, 0);

    return input;
}

void input_destroy(Input *input)
{
    input_device_destroy(input->mouse);
    input_device_destroy(input->keyboard);
    free(input);
}
