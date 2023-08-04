#include <engine/systems/transform_system.h>
#include <engine/component.h>
#include <core/log.h>
#include <core/macro.h>
#include <core/math.h>

ECS_COMPONENT_DECLARE(Walrus_Transform);
ECS_COMPONENT_DECLARE(Walrus_LocalTransform);

static void update_children(ecs_world_t *ecs, ecs_entity_t e, Walrus_Transform *p_world)
{
    ecs_filter_t *f  = ecs_filter_init(ecs, &(ecs_filter_desc_t){.terms = {{.id = ecs_id(Walrus_LocalTransform)},
                                                                           {.id = ecs_id(Walrus_Transform)},
                                                                           {.id = ecs_pair(EcsChildOf, e)}}});
    ecs_iter_t    it = ecs_filter_iter(ecs, f);
    while (ecs_filter_next(&it)) {
        Walrus_LocalTransform *locals = ecs_field(&it, Walrus_LocalTransform, 1);
        Walrus_LocalTransform *worlds = ecs_field(&it, Walrus_Transform, 2);
        for (i32 i = 0; i < it.count; ++i) {
            walrus_transform_mul(p_world, &locals[i], &worlds[i]);
        }
    }
    ecs_filter_fini(f);
}

static void on_transform_set(ecs_iter_t *it)
{
    Walrus_Transform *worlds = ecs_field(it, Walrus_Transform, 1);
    for (i32 i = 0; i < it->count; ++i) {
        ecs_entity_t e      = it->entities[i];
        ecs_entity_t parent = ecs_get_target(it->world, e, EcsChildOf, 0);
        if (parent != 0) {
            Walrus_Transform const *p_world = ecs_get(it->world, parent, Walrus_Transform);

            mat4 m, p_m;
            walrus_transform_compose(p_world, p_m);
            walrus_transform_compose(&worlds[i], m);
            glm_inv_tr(p_m);
            glm_mat4_mul(p_m, m, m);

            Walrus_Transform local;
            walrus_transform_decompose(&local, m);

            ecs_set(it->world, e, Walrus_LocalTransform,
                    {.trans = {local.trans[0], local.trans[1], local.trans[2]},
                     .rot   = {local.rot[0], local.rot[1], local.rot[2], local.rot[3]},
                     .scale = {local.scale[0], local.scale[1], local.scale[2]}});
        }
        else {
            ecs_set(it->world, e, Walrus_LocalTransform,
                    {.trans = {worlds[i].trans[0], worlds[i].trans[1], worlds[i].trans[2]},
                     .rot   = {worlds[i].rot[0], worlds[i].rot[1], worlds[i].rot[2], worlds[i].rot[3]},
                     .scale = {worlds[i].scale[0], worlds[i].scale[1], worlds[i].scale[2]}});
        }
        update_children(it->world, e, &worlds[i]);
    }
}

static void on_local_transform_set(ecs_iter_t *it)
{
    Walrus_LocalTransform *locals = ecs_field(it, Walrus_LocalTransform, 1);
    for (i32 i = 0; i < it->count; ++i) {
        ecs_entity_t      e      = it->entities[i];
        Walrus_Transform *world  = ecs_get_mut(it->world, e, Walrus_Transform);
        ecs_entity_t      parent = ecs_get_target(it->world, e, EcsChildOf, 0);
        if (parent != 0) {
            Walrus_Transform const *p_world = ecs_get(it->world, parent, Walrus_Transform);
            walrus_transform_mul(p_world, &locals[i], world);
        }
        else {
            *world = locals[i];
        }
        update_children(it->world, e, world);
    }
}

static void transform_system_init(Walrus_System *sys)
{
    ecs_world_t *ecs = sys->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_Transform);
    ECS_COMPONENT_DEFINE(ecs, Walrus_LocalTransform);

    ECS_OBSERVER(ecs, on_transform_set, EcsOnSet, Walrus_Transform);
    ECS_OBSERVER(ecs, on_local_transform_set, EcsOnSet, Walrus_LocalTransform);

    // TODO: serialize/deserialize test
    ecs_entity_t ecs_id(vec3);
    ecs_id(vec3) = ecs_array(ecs, {.entity = 0, .type = ecs_id(ecs_f32_t), .count = 3});

    ecs_entity_t ecs_id(versor);
    ecs_id(versor) = ecs_array(ecs, {.entity = 0, .type = ecs_id(ecs_f32_t), .count = 4});
    ecs_get_mut(ecs, ecs_id(versor), EcsComponent)->alignment = ECS_ALIGNOF(versor);
    ecs_modified(ecs, ecs_id(versor), EcsComponent);

    ecs_struct(ecs, {.entity  = ecs_id(Walrus_Transform),
                     .members = {
                         {.name = "translation", .type = ecs_id(vec3)},
                         {.name = "rotation", .type = ecs_id(versor)},
                         {.name = "scale", .type = ecs_id(vec3)},
                     }});
}

POLY_DEFINE_DERIVED(Walrus_System, void, transform_system_create, POLY_IMPL(on_system_init, transform_system_init))
