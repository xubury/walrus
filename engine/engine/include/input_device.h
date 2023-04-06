#pragma once

#include <type.h>
#include <cglm/cglm.h>

typedef struct _InputDevice InputDevice;

#define INTPUT_INVALID_ID UINT16_MAX;

InputDevice* input_create(i16 num_btns, i8 num_axes);

void input_destroy(InputDevice* device);

// Check if current button is pressed
bool input_pressed(InputDevice* device, i16 id);

// Return first button that was pressed, return INTPUT_INVALID_ID if no button is pressed
u16 input_any_pressed(InputDevice* device);

// Check if current button is released
bool input_released(InputDevice* device, i16 id);

// Return first button that was released, return INTPUT_INVALID_ID if no button is released
u16 input_any_released(InputDevice* device);

// Check if current button is down
bool input_down(InputDevice* device, i16 id);

// Return first button that was down, return INTPUT_INVALID_ID if no button is down
u16 input_any_down(InputDevice* device);

// Set button state
void input_set_button(InputDevice* device, i16 id, bool state, u8 modifiers);

// Check modifiers state
u8 input_modifiers(InputDevice* device);

// Get current input axis position
void input_axis(InputDevice* device, i8 id, vec3 out);

// Get relative axis movement from last tick
void input_relaxis(InputDevice* device, i8 id, vec3 out);

// Set axis state
void input_set_axis(InputDevice* device, i8 id, f32 x, f32 y, f32 z, u8 modifiers);

// Tick the state to next frame
void input_tick(InputDevice* device);
