#pragma once

#include <core/type.h>
#include <core/hash.h>
#include <engine/event.h>
#include <engine/input.h>

typedef void (*Walrus_AxisCallback)(f32 value, void *userdata);
typedef void (*Walrus_ActionCallback)(void *userdata);

typedef struct {
    Walrus_HashTable *mapping;
} Walrus_ControlMap;

void walrus_control_map_init(Walrus_ControlMap *control);
void walrus_control_map_shutdown(Walrus_ControlMap *control);

void walrus_control_add_axis_button(Walrus_ControlMap *control, char const *name, u8 device, u32 button, f32 scale,
                                    bool down);
void walrus_control_add_action_button(Walrus_ControlMap *control, char const *name, u8 device, u32 button);

void walrus_control_bind_axis(Walrus_ControlMap *control, char const *name, Walrus_AxisCallback func, void *userdata);
void walrus_control_bind_action(Walrus_ControlMap *control, char const *name, Walrus_ActionCallback func,
                                void *userdata);
void walrus_control_unbind(Walrus_ControlMap *control, char const *name);

void walrus_control_map_tick(Walrus_ControlMap *control, Walrus_Input *input);
