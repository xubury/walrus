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

    char *name;
} Mappings;

static Mappings *mapping_create(char const *name)
{
    Mappings *m    = walrus_new(Mappings, 1);
    m->name        = walrus_str_dup(name);
    m->btns        = walrus_array_create(sizeof(ButtonMapping), 0);
    m->axes        = walrus_array_create(sizeof(AxisMapping), 0);
    m->axis_func   = NULL;
    m->action_func = NULL;
    return m;
}

static void mapping_free(void *ptr)
{
    Mappings *m = ptr;
    walrus_array_destroy(m->btns);
    walrus_array_destroy(m->axes);
    walrus_free(m);
}

void walrus_input_map_init(Walrus_InputMap *map)
{
    map->table = walrus_hash_table_create_full(walrus_str_hash, walrus_str_equal, NULL, mapping_free);
}

void walrus_input_map_shutdown(Walrus_InputMap *map)
{
    walrus_hash_table_destroy(map->table);
}

void walrus_input_add_axis_axis(Walrus_InputMap *map, char const *name, u8 device, u8 axis, vec3 scale)
{
    Mappings *m = NULL;
    if (walrus_hash_table_contains(map->table, name)) {
        m = walrus_hash_table_lookup(map->table, name);
    }
    else {
        m = mapping_create(name);
        walrus_hash_table_insert(map->table, walrus_str_dup(name), m);
    }
    AxisMapping am;
    am.device = device;
    am.axis   = axis;
    glm_vec3_copy(scale, am.scale);
    walrus_array_append(m->axes, &am);
}

void walrus_input_add_axis_button(Walrus_InputMap *map, char const *name, u8 device, u32 button, vec3 scale, bool down)
{
    Mappings *m = NULL;
    if (walrus_hash_table_contains(map->table, name)) {
        m = walrus_hash_table_lookup(map->table, name);
    }
    else {
        m = mapping_create(name);
        walrus_hash_table_insert(map->table, m->name, m);
    }
    ButtonMapping btn;
    btn.device = device;
    btn.button = button;
    btn.mods   = 0;
    glm_vec3_copy(scale, btn.scale);
    btn.down   = down;
    btn.action = false;
    walrus_array_append(m->btns, &btn);
}

void walrus_input_add_action_button(Walrus_InputMap *map, char const *name, u8 device, u32 button)
{
    Mappings *m = NULL;
    if (walrus_hash_table_contains(map->table, name)) {
        m = walrus_hash_table_lookup(map->table, name);
    }
    else {
        m = mapping_create(name);
        walrus_hash_table_insert(map->table, m->name, m);
    }
    ButtonMapping btn;
    btn.device = device;
    btn.button = button;
    btn.mods   = 0;
    glm_vec3_zero(btn.scale);
    btn.down   = false;
    btn.action = true;
    walrus_array_append(m->btns, &btn);
}

void walrus_input_bind_axis(Walrus_InputMap *map, char const *name, Walrus_AxisCallback func)
{
    Mappings *m = NULL;
    if (walrus_hash_table_contains(map->table, name)) {
        m = walrus_hash_table_lookup(map->table, name);
    }
    else {
        m = mapping_create(name);
        walrus_hash_table_insert(map->table, m->name, m);
    }
    m->axis_func = func;
}

void walrus_input_bind_action(Walrus_InputMap *map, char const *name, Walrus_ActionCallback func)
{
    Mappings *m = NULL;
    if (walrus_hash_table_contains(map->table, name)) {
        m = walrus_hash_table_lookup(map->table, name);
    }
    else {
        m = mapping_create(name);
        walrus_hash_table_insert(map->table, m->name, m);
    }
    m->action_func = func;
}

void walrus_input_unbind(Walrus_InputMap *map, char const *name)
{
    if (walrus_hash_table_contains(map->table, name)) {
        Mappings *m    = walrus_hash_table_lookup(map->table, name);
        m->action_func = NULL;
        m->axis_func   = NULL;
    }
}

void walrus_input_clear(Walrus_InputMap *map, char const *name)
{
    walrus_hash_table_remove(map->table, name);
}

void foreach_axis_control(void const *key, void *value, void *userdata)
{
    walrus_unused(key);
    Walrus_Input   *input = walrus_engine_vars()->input;
    Mappings const *m     = value;
    if (m->axis_func == NULL) {
        return;
    }

    u32 num_btns = walrus_array_len(m->btns);
    for (u32 i = 0; i < num_btns; ++i) {
        bool                trigger = false;
        Walrus_InputDevice *device  = NULL;

        ButtonMapping *btn = walrus_array_get(m->btns, i);

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
            m->axis_func(btn->scale, userdata);
        }
    }
    u32 num_axes = walrus_array_len(m->axes);
    for (u32 i = 0; i < num_axes; ++i) {
        bool                trigger = false;
        Walrus_InputDevice *device  = NULL;

        AxisMapping *axis = walrus_array_get(m->axes, i);

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
            m->axis_func(rel, userdata);
        }
    }
}

void foreach_action_control(void const *key, void *value, void *userdata)
{
    walrus_unused(key);
    Walrus_Input   *input = walrus_engine_vars()->input;
    Mappings const *m     = value;
    if (m->action_func == NULL) {
        return;
    }

    u32 size = walrus_array_len(m->btns);
    for (u32 i = 0; i < size; ++i) {
        bool                trigger = false;
        Walrus_InputDevice *device  = NULL;

        ButtonMapping *btn = walrus_array_get(m->btns, i);

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
            m->action_func(userdata);
        }
    }
}

void walrus_input_map_tick(Walrus_InputMap *map, void *userdata)
{
    walrus_hash_table_foreach(map->table, foreach_axis_control, userdata);
    walrus_hash_table_foreach(map->table, foreach_action_control, userdata);
}
