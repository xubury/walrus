#include <engine/control_map.h>
#include <engine/event.h>
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
    f32  scale;
    bool action;
    bool down;
} ButtonMapping;

typedef struct {
    Walrus_Array *btns;
    Walrus_Array *axes;

    Walrus_AxisCallback   axis_func;
    Walrus_ActionCallback action_func;
    void                 *userdata;
} Mappings;

static Mappings *mapping_create(void)
{
    Mappings *mapping    = walrus_new(Mappings, 1);
    mapping->btns        = walrus_array_create(sizeof(ButtonMapping), 0);
    mapping->axis_func   = NULL;
    mapping->action_func = NULL;
    mapping->userdata    = NULL;
    return mapping;
}

static void mapping_free(void *ptr)
{
    Mappings *mapping = ptr;
    walrus_array_destroy(mapping->btns);
    walrus_free(mapping);
}

void walrus_control_map_init(Walrus_ControlMap *control)
{
    control->mapping = walrus_hash_table_create_full(walrus_str_hash, walrus_str_equal,
                                                     (Walrus_KeyDestroyFunc)walrus_str_free, mapping_free);
}

void walrus_control_map_shutdown(Walrus_ControlMap *control)
{
    walrus_hash_table_destroy(control->mapping);
}

void walrus_control_add_axis_button(Walrus_ControlMap *control, char const *name, u8 device, u32 button, f32 scale,
                                    bool down)
{
    Mappings *mapping = NULL;
    if (walrus_hash_table_contains(control->mapping, name)) {
        mapping = walrus_hash_table_lookup(control->mapping, name);
    }
    else {
        mapping = mapping_create();
        walrus_hash_table_insert(control->mapping, walrus_str_dup(name), mapping);
    }
    ButtonMapping btn;
    btn.device = device;
    btn.button = button;
    btn.mods   = 0;
    btn.scale  = scale;
    btn.down   = down;
    btn.action = false;
    walrus_array_append(mapping->btns, &btn);
}

void walrus_control_add_action_button(Walrus_ControlMap *control, char const *name, u8 device, u32 button)
{
    Mappings *mapping = NULL;
    if (walrus_hash_table_contains(control->mapping, name)) {
        mapping = walrus_hash_table_lookup(control->mapping, name);
    }
    else {
        mapping = mapping_create();
        walrus_hash_table_insert(control->mapping, walrus_str_dup(name), mapping);
    }
    ButtonMapping btn;
    btn.device = device;
    btn.button = button;
    btn.mods   = 0;
    btn.scale  = 0;
    btn.down   = false;
    btn.action = true;
    walrus_array_append(mapping->btns, &btn);
}

void walrus_control_bind_axis(Walrus_ControlMap *control, char const *name, Walrus_AxisCallback func, void *userdata)
{
    Mappings *mapping = NULL;
    if (walrus_hash_table_contains(control->mapping, name)) {
        mapping = walrus_hash_table_lookup(control->mapping, name);
    }
    else {
        mapping = mapping_create();
        walrus_hash_table_insert(control->mapping, walrus_str_dup(name), mapping);
    }
    mapping->axis_func = func;
    mapping->userdata  = userdata;
}

void walrus_control_bind_action(Walrus_ControlMap *control, char const *name, Walrus_ActionCallback func,
                                void *userdata)
{
    Mappings *mapping = NULL;
    if (walrus_hash_table_contains(control->mapping, name)) {
        mapping = walrus_hash_table_lookup(control->mapping, name);
    }
    else {
        mapping = mapping_create();
        walrus_hash_table_insert(control->mapping, walrus_str_dup(name), mapping);
    }
    mapping->action_func = func;
    mapping->userdata    = userdata;
}

void walrus_control_unbind(Walrus_ControlMap *control, char const *name)
{
    if (walrus_hash_table_contains(control->mapping, name)) {
        Mappings *mapping    = walrus_hash_table_lookup(control->mapping, name);
        mapping->action_func = NULL;
        mapping->axis_func   = NULL;
    }
}

void walrus_control_clear(Walrus_ControlMap *control, char const *name)
{
    walrus_hash_table_remove(control->mapping, name);
}

void foreach_axis_control(void const *key, void const *value, void *userdata)
{
    walrus_unused(key);
    Mappings const *mapping = value;
    Walrus_Input   *input   = userdata;
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
            mapping->axis_func(btn->scale, mapping->userdata);
        }
    }
}

void foreach_action_control(void const *key, void const *value, void *userdata)
{
    walrus_unused(key);
    Mappings const *mapping = value;
    Walrus_Input   *input   = userdata;
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
            mapping->action_func(mapping->userdata);
        }
    }
}

void walrus_control_map_tick(Walrus_ControlMap *control, Walrus_Input *input)
{
    walrus_hash_table_foreach(control->mapping, foreach_axis_control, input);
    walrus_hash_table_foreach(control->mapping, foreach_action_control, input);
}
