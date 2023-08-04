#include <engine/systems/model_system.h>
#include <engine/component.h>
#include <engine/model.h>
#include <core/hash.h>
#include <core/string.h>
#include <core/macro.h>
#include <core/memory.h>
#include <core/assert.h>

ECS_COMPONENT_DECLARE(Walrus_ModelRef);

static void on_model_set(ecs_iter_t *it)
{
    Walrus_ModelRef *refs = ecs_field(it, Walrus_ModelRef, 1);

    for (i32 i = 0; i < it->count; ++i) {
        ++(*refs[i].ref_count);
    }
}

static void on_model_unset(ecs_iter_t *it)
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

static void model_system_init(Walrus_System *sys)
{
    ModelSystem *model_sys = poly_cast(sys, ModelSystem);

    ecs_world_t *ecs = sys->ecs;

    ECS_COMPONENT_DEFINE(ecs, Walrus_ModelRef);

    ecs_observer_init(ecs, &(ecs_observer_desc_t const){.events       = {EcsOnSet},
                                                        .entity       = ecs_entity(ecs, {0}),
                                                        .callback     = on_model_set,
                                                        .filter.terms = {{.id = ecs_id(Walrus_ModelRef)}}});
    ecs_observer_init(ecs, &(ecs_observer_desc_t const){.events       = {EcsUnSet},
                                                        .entity       = ecs_entity(ecs, {0}),
                                                        .callback     = on_model_unset,
                                                        .filter.terms = {{.id = ecs_id(Walrus_ModelRef)}}});

    model_sys->table = walrus_hash_table_create(walrus_str_hash, walrus_str_equal);
}

static void model_system_shutdown(Walrus_System *sys)
{
    ModelSystem *model_sys = poly_cast(sys, ModelSystem);
    walrus_hash_table_destroy(model_sys->table);
}

void walrus_model_system_load_from_file(Walrus_System *sys, char const *name, char const *filename)
{
    ModelSystem *model_sys = poly_cast(sys, ModelSystem);
    if (!walrus_hash_table_contains(model_sys->table, name)) {
        ecs_world_t *ecs = sys->ecs;
        ecs_entity_t e   = ecs_set(
            ecs, 0, Walrus_ModelRef,
            {.name = walrus_str_dup(name), .path = walrus_str_dup(filename), .ref_count = walrus_malloc0(sizeof(i32))});

        Walrus_ModelRef *ref = ecs_get_mut(ecs, e, Walrus_ModelRef);
        if (walrus_model_load_from_file(&ref->model, ref->path) == WR_MODEL_SUCCESS) {
            ecs_modified(ecs, e, Walrus_ModelRef);
            walrus_hash_table_insert(model_sys->table, ref->name, walrus_val_to_ptr(e));
        }
        else {
            ecs_delete(ecs, e);
        }
    }
}

bool walrus_model_system_unload(Walrus_System *sys, char const *name)
{
    ModelSystem *model_sys = poly_cast(sys, ModelSystem);
    if (walrus_hash_table_contains(model_sys->table, name)) {
        ecs_entity_t e   = walrus_ptr_to_val(walrus_hash_table_lookup(model_sys->table, name));
        ecs_world_t *ecs = sys->ecs;
        ecs_delete(ecs, e);

        walrus_hash_table_remove(model_sys->table, name);

        return true;
    }

    return false;
}

ecs_entity_t walrus_model_instantiate(Walrus_System *sys, char const *name, vec3 const trans, versor const rot,
                                      vec3 const scale)
{
    ModelSystem *model_sys = poly_cast(sys, ModelSystem);
    ecs_world_t *ecs       = sys->ecs;
    if (walrus_hash_table_contains(model_sys->table, name)) {
        ecs_entity_t base_model = walrus_ptr_to_val(walrus_hash_table_lookup(model_sys->table, name));

        ecs_entity_t e = ecs_new_id(ecs);
        ecs_set(ecs, e, Walrus_Transform,
                {.trans = {trans[0], trans[1], trans[2]},
                 .rot   = {rot[0], rot[1], rot[2], rot[3]},
                 .scale = {scale[0], scale[1], scale[2]}});
        ecs_add_pair(ecs, e, EcsIsA, base_model);

        return e;
    }
    return 0;
}

POLY_DEFINE_DERIVED(Walrus_System, ModelSystem, model_system_create, POLY_IMPL(on_system_init, model_system_init),
                    POLY_IMPL(on_system_shutdown, model_system_shutdown))
