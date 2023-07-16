#include <engine/systems/model_system.h>
#include <engine/component.h>
#include <engine/engine.h>
#include <engine/model.h>
#include <core/hash.h>
#include <core/string.h>
#include <core/macro.h>
#include <core/memory.h>
#include <core/assert.h>

ECS_COMPONENT_DECLARE(Walrus_ModelRef);

typedef struct {
    Walrus_HashTable *model_table;
} ModelSystem;

static ModelSystem s_system;

void model_destroy(void *ptr)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ecs_entity_t e   = walrus_ptr_to_val(ptr);
    ecs_delete(ecs, e);
}

static void on_model_add(ecs_iter_t *it)
{
    Walrus_ModelRef *refs = ecs_field(it, Walrus_ModelRef, 1);

    for (i32 i = 0; i < it->count; ++i) {
        ++(*refs[i].ref_count);
    }
}

static void on_model_remove(ecs_iter_t *it)
{
    Walrus_ModelRef *refs = ecs_field(it, Walrus_ModelRef, 1);

    for (i32 i = 0; i < it->count; ++i) {
        --(*refs[i].ref_count);
        if (*refs[i].ref_count == 0) {
            walrus_str_free(refs[i].name);
            walrus_str_free(refs[i].path);
            walrus_model_shutdown(&refs[i].model);
        }
    }
}

void walrus_model_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_ModelRef);

    ecs_observer_init(ecs, &(ecs_observer_desc_t const){.events       = {EcsOnSet},
                                                        .entity       = ecs_entity(ecs, {0}),
                                                        .callback     = on_model_add,
                                                        .filter.terms = {{.id = ecs_id(Walrus_ModelRef)}}});
    ecs_observer_init(ecs, &(ecs_observer_desc_t const){.events       = {EcsOnRemove},
                                                        .entity       = ecs_entity(ecs, {0}),
                                                        .callback     = on_model_remove,
                                                        .filter.terms = {{.id = ecs_id(Walrus_ModelRef)}}});

    s_system.model_table = walrus_hash_table_create_full(walrus_str_hash, walrus_str_equal, NULL, model_destroy);
}

void walrus_model_system_shutdown(void)
{
    walrus_hash_table_destroy(s_system.model_table);
}

void walrus_model_system_load_from_file(char const *name, char const *filename)
{
    ecs_entity_t e;
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    if (walrus_hash_table_contains(s_system.model_table, name)) {
        e = walrus_ptr_to_val(walrus_hash_table_lookup(s_system.model_table, name));
    }
    else {
        e = ecs_set(
            ecs, 0, Walrus_ModelRef,
            {.name = walrus_str_dup(name), .path = walrus_str_dup(filename), .ref_count = walrus_malloc0(sizeof(i32))});

        Walrus_ModelRef *ref = ecs_get_mut(ecs, e, Walrus_ModelRef);
        if (walrus_model_load_from_file(&ref->model, filename) == WR_MODEL_SUCCESS) {
            walrus_hash_table_insert(s_system.model_table, ref->name, walrus_val_to_ptr(e));
        }
        else {
            ecs_delete(ecs, e);
        }
    }
}

bool walrus_model_system_unload(char const *name)
{
    if (walrus_hash_table_contains(s_system.model_table, name)) {
        walrus_hash_table_remove(s_system.model_table, name);

        return true;
    }

    return false;
}

ecs_entity_t walrus_model_instantiate(char const *name, vec3 const trans, versor const rot, vec3 const scale)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    if (walrus_hash_table_contains(s_system.model_table, name)) {
        ecs_entity_t base_model = walrus_ptr_to_val(walrus_hash_table_lookup(s_system.model_table, name));
        ecs_entity_t e          = ecs_new_w_pair(ecs, EcsIsA, base_model);
        ecs_set(ecs, e, Walrus_Transform,
                {.trans = {trans[0], trans[1], trans[2]},
                 .rot   = {rot[0], rot[1], rot[2], rot[3]},
                 .scale = {scale[0], scale[1], scale[2]}});

        return e;
    }
    return 0;
}
