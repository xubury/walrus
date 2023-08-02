#include <engine/systems/transform_system.h>
#include <core/log.h>
#include <core/macro.h>
#include <core/math.h>

ECS_COMPONENT_DECLARE(Walrus_Transform);
ECS_COMPONENT_DECLARE(Walrus_LocalTransform);

static void transform_tick(ecs_iter_t *it)
{
    Walrus_Transform *worlds   = ecs_field(it, Walrus_Transform, 1);
    Walrus_Transform *p_worlds = ecs_field(it, Walrus_Transform, 2);
    Walrus_Transform *locals   = ecs_field(it, Walrus_LocalTransform, 3);
    for (i32 i = 0; i < it->count; ++i) {
        if (p_worlds == NULL) {
            locals[i] = worlds[i];
        }
        else {
            walrus_transform_mul(&p_worlds[i], &locals[i], &worlds[i]);
        }
    }
}

static void on_transform_add(ecs_iter_t *it)
{
    Walrus_Transform *world = ecs_field(it, Walrus_Transform, 1);
    ecs_entity_t      child = it->entities[0];
    if (ecs_has(it->world, child, Walrus_LocalTransform)) {
        return;
    }
    ecs_entity_t parent = ecs_get_target(it->world, child, EcsChildOf, 0);
    if (parent != 0) {
        Walrus_Transform const *p_world = ecs_get(it->world, parent, Walrus_Transform);

        mat4 m, p_m;
        walrus_transform_compose(p_world, p_m);
        walrus_transform_compose(world, m);
        glm_inv_tr(p_m);
        glm_mat4_mul(p_m, m, m);

        Walrus_Transform local;
        walrus_transform_decompose(&local, m);

        ecs_set(it->world, child, Walrus_LocalTransform,
                {.trans = {local.trans[0], local.trans[1], local.trans[2]},
                 .rot   = {local.rot[0], local.rot[1], local.rot[2], local.rot[3]},
                 .scale = {local.scale[0], local.scale[1], local.scale[2]}});
    }
    else {
        ecs_set(it->world, child, Walrus_LocalTransform,
                {.trans = {world->trans[0], world->trans[1], world->trans[2]},
                 .rot   = {world->rot[0], world->rot[1], world->rot[2], world->rot[3]},
                 .scale = {world->scale[0], world->scale[1], world->scale[2]}});
    }
}

static void transform_system_init(Walrus_System *sys)
{
    ecs_world_t *ecs = sys->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_Transform);
    ECS_COMPONENT_DEFINE(ecs, Walrus_LocalTransform);

    ECS_OBSERVER(ecs, on_transform_add, EcsOnSet, Walrus_Transform);

    ecs_system(ecs,
               {.entity = ecs_entity(ecs, {.add = {ecs_dependson(EcsOnUpdate)}}),
                .query.filter.terms =
                    {
                        {.id = ecs_id(Walrus_Transform)},
                        {.id = ecs_id(Walrus_Transform), .src = {.flags = EcsCascade | EcsParent}, .oper = EcsOptional},
                        {.id = ecs_id(Walrus_LocalTransform)},
                    },
                .callback = transform_tick});

    // TODO: serialize/deserialize test
    ECS_COMPONENT(ecs, vec3);
    ecs_struct(ecs, {.entity  = ecs_id(vec3),
                     .members = {{.name = "x", .type = ecs_id(ecs_f32_t)},
                                 {.name = "y", .type = ecs_id(ecs_f32_t)},
                                 {.name = "z", .type = ecs_id(ecs_f32_t)}}});
    ecs_struct(ecs, {.entity  = ecs_id(Walrus_Transform),
                     .members = {
                         {.name = "trans", .type = ecs_id(vec3)},
                     }});
}

POLY_DEFINE_DERIVED(Walrus_System, TransformSystem, transform_system_create,
                    POLY_IMPL(on_system_init, transform_system_init))
