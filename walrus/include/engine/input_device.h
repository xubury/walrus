#pragma once

#include <core/type.h>
#include <cglm/cglm.h>

typedef struct _InputDevice Walrus_InputDevice;

#define WR_INPUT_INVALID_ID UINT16_MAX;

Walrus_InputDevice* walrus_input_create(i16 num_btns, i8 num_axes);

void walrus_input_destroy(Walrus_InputDevice* device);

// Check if current button is pressed
bool walrus_input_pressed(Walrus_InputDevice* device, i16 id);

// Return first button that was pressed, return INTPUT_INVALID_ID if no button is pressed
u16 walrus_input_any_pressed(Walrus_InputDevice* device);

// Check if current button is released
bool walrus_input_released(Walrus_InputDevice* device, i16 id);

// Return first button that was released, return INTPUT_INVALID_ID if no button is released
u16 walrus_input_any_released(Walrus_InputDevice* device);

// Check if current button is down
bool walrus_input_down(Walrus_InputDevice* device, i16 id);

// Return first button that was down, return INTPUT_INVALID_ID if no button is down
u16 walrus_input_any_down(Walrus_InputDevice* device);

// Set button state
void walrus_input_set_button(Walrus_InputDevice* device, i16 id, bool state, u8 modifiers);

// Check modifiers state
u8 walrus_input_modifiers(Walrus_InputDevice* device);

// Get current input axis position
void walrus_input_axis(Walrus_InputDevice* device, i8 id, vec3 out);

// Get relative axis movement from last tick
void walrus_input_relaxis(Walrus_InputDevice* device, i8 id, vec3 out);

// Set axis state
void walrus_input_set_axis(Walrus_InputDevice* device, i8 id, f32 x, f32 y, f32 z, u8 modifiers);

// Tick the state to next frame
void walrus_input_tick(Walrus_InputDevice* device);
