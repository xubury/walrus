#pragma once

#include <core/type.h>
#include <core/hash.h>

#include <cglm/types.h>

typedef struct {
    Walrus_HashTable *table;
} Walrus_InputMap;

void walrus_input_map_init(Walrus_InputMap *map);
void walrus_input_map_shutdown(Walrus_InputMap *map);

void walrus_input_add_axis_axis(Walrus_InputMap *map, char const *name, u8 device, u8 axis, vec3 scale);
void walrus_input_add_axis_button(Walrus_InputMap *map, char const *name, u8 device, u32 button, vec3 scale, bool down);
void walrus_input_add_action_button(Walrus_InputMap *map, char const *name, u8 device, u32 button);

void walrus_input_clear(Walrus_InputMap *map, char const *name);

bool walrus_input_get_axis(Walrus_InputMap *map, char const *name, vec3 scale);
bool walrus_input_get_action(Walrus_InputMap *map, char const *name);
