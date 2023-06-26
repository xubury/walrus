#include <engine/systems/model_system.h>
#include <engine/systems/transform_system.h>
#include <engine/systems/animator_system.h>
#include <engine/engine.h>
#include <engine/model.h>
#include <core/hash.h>
#include <core/string.h>
#include <core/macro.h>

ECS_COMPONENT_DECLARE(Walrus_Model);

typedef struct {
    Walrus_HashTable *model_table;
} ModelSystem;

static ModelSystem s_system;

void model_shutdown(void *ptr)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ecs_entity_t e   = walrus_ptr_to_val(ptr);
    walrus_model_shutdown(ecs_get_mut(ecs, e, Walrus_Model));
    ecs_delete(ecs, e);
}

void walrus_model_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_Model);
    s_system.model_table = walrus_hash_table_create_full(walrus_str_hash, walrus_str_equal,
                                                         (Walrus_KeyDestroyFunc)walrus_str_free, model_shutdown);
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
        e = ecs_set(ecs, 0, Walrus_Model, {0});

        Walrus_Model *model = ecs_get_mut(ecs, e, Walrus_Model);
        if (walrus_model_load_from_file(model, filename) == WR_MODEL_SUCCESS) {
            walrus_hash_table_insert(s_system.model_table, walrus_str_dup(name), walrus_val_to_ptr(e));
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
