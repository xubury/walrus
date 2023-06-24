#pragma once

#include <core/type.h>
#include <core/hash.h>

#include <cglm/types.h>

typedef void (*Walrus_AxisCallback)(vec3 value, void *userdata);
typedef void (*Walrus_ActionCallback)(void *userdata);

typedef struct {
    Walrus_HashTable *mapping;
} Walrus_ControlMap;

void walrus_control_map_init(Walrus_ControlMap *map);
void walrus_control_map_shutdown(Walrus_ControlMap *map);

void walrus_control_add_axis_axis(Walrus_ControlMap *map, char const *name, u8 device, u8 axis, vec3 scale);
void walrus_control_add_axis_button(Walrus_ControlMap *map, char const *name, u8 device, u32 button, vec3 scale,
                                    bool down);
void walrus_control_add_action_button(Walrus_ControlMap *map, char const *name, u8 device, u32 button);

void walrus_control_bind_axis(Walrus_ControlMap *map, char const *name, Walrus_AxisCallback func);
void walrus_control_bind_action(Walrus_ControlMap *map, char const *name, Walrus_ActionCallback func);

void walrus_control_unbind(Walrus_ControlMap *map, char const *name);
void walrus_control_clear(Walrus_ControlMap *map, char const *name);

void walrus_control_map_tick(Walrus_ControlMap *map, void *userdata);
