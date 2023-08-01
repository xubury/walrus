#include <engine/systems/animator_system.h>
#include <engine/systems/model_system.h>
#include <engine/animator.h>
#include <core/macro.h>

ECS_COMPONENT_DECLARE(Walrus_Animator);

static void animator_tick(ecs_iter_t *it)
{
    Walrus_Animator *animators = ecs_field(it, Walrus_Animator, 1);
    Walrus_ModelRef *refs      = ecs_field(it, Walrus_ModelRef, 2);

    for (i32 i = 0; i < it->count; ++i) {
        walrus_animator_tick(&animators[i], &refs[i].model, it->delta_time);
    }
}

static void on_animator_model_add(ecs_iter_t *it)
{
    Walrus_Animator *animator = ecs_field(it, Walrus_Animator, 1);
    Walrus_ModelRef *ref      = ecs_field(it, Walrus_ModelRef, 2);

    walrus_animator_init(animator);
    walrus_animator_bind(animator, &ref->model);
    walrus_animator_play(animator, 0);
}

static void on_animator_remove(ecs_iter_t *it)
{
    Walrus_Animator *animator = ecs_field(it, Walrus_Animator, 1);
    walrus_animator_shutdown(animator);
}

static void animator_system_init(Walrus_System *sys)
{
    ecs_world_t *ecs = sys->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_Animator);

    ECS_SYSTEM(ecs, animator_tick, EcsOnUpdate, Walrus_Animator, Walrus_ModelRef);

    ECS_OBSERVER(ecs, on_animator_model_add, EcsOnSet, Walrus_Animator, Walrus_ModelRef);
    ECS_OBSERVER(ecs, on_animator_remove, EcsOnRemove, Walrus_Animator);
}

POLY_DEFINE_DERIVED(Walrus_System, AnimatorSystem, animator_system_create,
                    POLY_IMPL(on_system_init, animator_system_init))
