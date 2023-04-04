#pragma once

#include <type.h>
#include <cglm/cglm.h>

typedef struct _InputDevice InputDevice;

#define INTPUT_INVALID_ID UINT16_MAX;

// Check if current button is pressed
bool pressed(InputDevice* device, u16 id);

// Return first button that was pressed, return INTPUT_INVALID_ID if no button is pressed
u16 any_pressed(InputDevice* device);

// Check if current button is released
bool released(InputDevice* device, u16 id);

// Return first button that was released, return INTPUT_INVALID_ID if no button is released
u16 any_released(InputDevice* device);

// Check if current button is down
bool down(InputDevice* device, u16 id);

// Return first button that was down, return INTPUT_INVALID_ID if no button is down
u16 any_down(InputDevice* device);

// Set button state
void set_button(InputDevice* device, u16 id, bool state, u8 modifiers);

// Check modifiers state
u8 modifiers(InputDevice* device);

// Get current input axis position
float* axis(InputDevice* device, u8 id);

// Get relative axis movement from last tick
float* axis_relative(InputDevice* device, u8 id);

// Set axis state
void set_axis(InputDevice* device, u8 id, float x, float y, float z);

// Tick the state to next frame
void tick(InputDevice* device);
