#include <engine/input_map.h>
#include <engine/event.h>
#include <engine/engine.h>
#include <core/memory.h>
#include <core/string.h>
#include <core/macro.h>
#include <core/array.h>
#include <core/log.h>
#include <core/math.h>

typedef struct {
    u8   device;
    u32  button;
    u8   mods;
    vec3 scale;
    bool action;
    bool down;
} ButtonMapping;

typedef struct {
    u8   device;
    u8   axis;
    vec3 scale;
} AxisMapping;

typedef struct {
    Walrus_Array *btns;
    Walrus_Array *axes;

    Walrus_AxisCallback   axis_func;
    Walrus_ActionCallback action_func;
} Mappings;

static Mappings *mapping_create(void)
{
    Mappings *mapping    = walrus_new(Mappings, 1);
    mapping->btns        = walrus_array_create(sizeof(ButtonMapping), 0);
    mapping->axes        = walrus_array_create(sizeof(AxisMapping), 0);
    mapping->axis_func   = NULL;
    mapping->action_func = NULL;
    return mapping;
}

static void mapping_free(void *ptr)
{
    Mappings *mapping = ptr;
    walrus_array_destroy(mapping->btns);
    walrus_array_destroy(mapping->axes);
    walrus_free(mapping);
}

void walrus_input_map_init(Walrus_InputMap *map)
{
    map->mapping = walrus_hash_table_create_full(walrus_str_hash, walrus_str_equal,
                                                 (Walrus_KeyDestroyFunc)walrus_str_free, mapping_free);
}

void walrus_input_map_shutdown(Walrus_InputMap *map)
{
    walrus_hash_table_destroy(map->mapping);
}

void walrus_input_add_axis_axis(Walrus_InputMap *map, char const *name, u8 device, u8 axis, vec3 scale)
{
    Mappings *mapping = NULL;
    if (walrus_hash_table_contains(map->mapping, name)) {
        mapping = walrus_hash_table_lookup(map->mapping, name);
    }
    else {
        mapping = mapping_create();
        walrus_hash_table_insert(map->mapping, walrus_str_dup(name), mapping);
    }
    AxisMapping am;
    am.device = device;
    am.axis   = axis;
    glm_vec3_copy(scale, am.scale);
    walrus_array_append(mapping->axes, &am);
}

void walrus_input_add_axis_button(Walrus_InputMap *map, char const *name, u8 device, u32 button, vec3 scale,
                                    bool down)
{
    Mappings *mapping = NULL;
    if (walrus_hash_table_contains(map->mapping, name)) {
        mapping = walrus_hash_table_lookup(map->mapping, name);
    }
    else {
        mapping = mapping_create();
        walrus_hash_table_insert(map->mapping, walrus_str_dup(name), mapping);
    }
    ButtonMapping btn;
    btn.device = device;
    btn.button = button;
    btn.mods   = 0;
    glm_vec3_copy(scale, btn.scale);
    btn.down   = down;
    btn.action = false;
    walrus_array_append(mapping->btns, &btn);
}

void walrus_input_add_action_button(Walrus_InputMap *map, char const *name, u8 device, u32 button)
{
    Mappings *mapping = NULL;
    if (walrus_hash_table_contains(map->mapping, name)) {
        mapping = walrus_hash_table_lookup(map->mapping, name);
    }
    else {
        mapping = mapping_create();
        walrus_hash_table_insert(map->mapping, walrus_str_dup(name), mapping);
    }
    ButtonMapping btn;
    btn.device = device;
    btn.button = button;
    btn.mods   = 0;
    glm_vec3_zero(btn.scale);
    btn.down   = false;
    btn.action = true;
    walrus_array_append(mapping->btns, &btn);
}

void walrus_input_bind_axis(Walrus_InputMap *map, char const *name, Walrus_AxisCallback func)
{
    Mappings *mapping = NULL;
    if (walrus_hash_table_contains(map->mapping, name)) {
        mapping = walrus_hash_table_lookup(map->mapping, name);
    }
    else {
        mapping = mapping_create();
        walrus_hash_table_insert(map->mapping, walrus_str_dup(name), mapping);
    }
    mapping->axis_func = func;
}

void walrus_input_bind_action(Walrus_InputMap *map, char const *name, Walrus_ActionCallback func)
{
    Mappings *mapping = NULL;
    if (walrus_hash_table_contains(map->mapping, name)) {
        mapping = walrus_hash_table_lookup(map->mapping, name);
    }
    else {
        mapping = mapping_create();
        walrus_hash_table_insert(map->mapping, walrus_str_dup(name), mapping);
    }
    mapping->action_func = func;
}

void walrus_input_unbind(Walrus_InputMap *map, char const *name)
{
    if (walrus_hash_table_contains(map->mapping, name)) {
        Mappings *mapping    = walrus_hash_table_lookup(map->mapping, name);
        mapping->action_func = NULL;
        mapping->axis_func   = NULL;
    }
}

void walrus_input_clear(Walrus_InputMap *map, char const *name)
{
    walrus_hash_table_remove(map->mapping, name);
}

void foreach_axis_control(void const *key, void *value, void *userdata)
{
    walrus_unused(key);
    Walrus_Input   *input   = walrus_engine_vars()->input;
    Mappings const *mapping = value;
    if (mapping->axis_func == NULL) {
        return;
    }

    u32 num_btns = walrus_array_len(mapping->btns);
    for (u32 i = 0; i < num_btns; ++i) {
        bool                trigger = false;
        Walrus_InputDevice *device  = NULL;

        ButtonMapping *btn = walrus_array_get(mapping->btns, i);

        if (btn->action) {
            continue;
        }

        if (btn->device == WR_INPUT_KEYBOARD) {
            device = input->keyboard;
        }
        else if (btn->device == WR_INPUT_MOUSE) {
            device = input->mouse;
        }

        if (device) {
            if (btn->down) {
                trigger = walrus_input_down(device, btn->button);
            }
            else {
                trigger = walrus_input_pressed(device, btn->button);
            }
        }
        if (trigger) {
            mapping->axis_func(btn->scale, userdata);
        }
    }
    u32 num_axes = walrus_array_len(mapping->axes);
    for (u32 i = 0; i < num_axes; ++i) {
        bool                trigger = false;
        Walrus_InputDevice *device  = NULL;

        AxisMapping *axis = walrus_array_get(mapping->axes, i);

        if (axis->device == WR_INPUT_MOUSE) {
            device = input->mouse;
        }

        vec3 rel;
        if (device) {
            walrus_input_relaxis(device, axis->axis, &rel[0], &rel[1], &rel[2]);
            trigger = glm_vec3_norm2(rel) > 0;
        }
        if (trigger) {
            glm_vec3_mul(rel, axis->scale, rel);
            mapping->axis_func(rel, userdata);
        }
    }
}

void foreach_action_control(void const *key, void *value, void *userdata)
{
    walrus_unused(key);
    Walrus_Input   *input   = walrus_engine_vars()->input;
    Mappings const *mapping = value;
    if (mapping->action_func == NULL) {
        return;
    }

    u32 size = walrus_array_len(mapping->btns);
    for (u32 i = 0; i < size; ++i) {
        bool                trigger = false;
        Walrus_InputDevice *device  = NULL;

        ButtonMapping *btn = walrus_array_get(mapping->btns, i);

        if (!btn->action) {
            continue;
        }

        if (btn->device == WR_INPUT_KEYBOARD) {
            device = input->keyboard;
        }
        else if (btn->device == WR_INPUT_MOUSE) {
            device = input->mouse;
        }

        if (device) {
            trigger = walrus_input_pressed(device, btn->button);
        }
        if (trigger) {
            mapping->action_func(userdata);
        }
    }
}

void walrus_input_map_tick(Walrus_InputMap *map, void *userdata)
{
    walrus_hash_table_foreach(map->mapping, foreach_axis_control, userdata);
    walrus_hash_table_foreach(map->mapping, foreach_action_control, userdata);
}
