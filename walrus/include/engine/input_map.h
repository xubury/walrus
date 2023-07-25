#pragma once

#include <core/type.h>
#include <core/hash.h>

#include <cglm/types.h>

typedef void (*Walrus_AxisCallback)(vec3 value, void *userdata);
typedef void (*Walrus_ActionCallback)(void *userdata);

typedef struct {
    Walrus_HashTable *table;
} Walrus_InputMap;

void walrus_input_map_init(Walrus_InputMap *map);
void walrus_input_map_shutdown(Walrus_InputMap *map);

void walrus_input_add_axis_axis(Walrus_InputMap *map, char const *name, u8 device, u8 axis, vec3 scale);
void walrus_input_add_axis_button(Walrus_InputMap *map, char const *name, u8 device, u32 button, vec3 scale, bool down);
void walrus_input_add_action_button(Walrus_InputMap *map, char const *name, u8 device, u32 button);

void walrus_input_bind_axis(Walrus_InputMap *map, char const *name, Walrus_AxisCallback func);
void walrus_input_bind_action(Walrus_InputMap *map, char const *name, Walrus_ActionCallback func);

void walrus_input_unbind(Walrus_InputMap *map, char const *name);
void walrus_input_clear(Walrus_InputMap *map, char const *name);

void walrus_input_map_tick(Walrus_InputMap *map, void *userdata);
