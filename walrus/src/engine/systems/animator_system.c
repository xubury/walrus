#include <engine/systems/animator_system.h>
#include <engine/engine.h>
#include <engine/animator.h>

static void animator_tick(ecs_iter_t *it)
{
    Walrus_Animator *animators = ecs_field(it, Walrus_Animator, 1);
    Walrus_Model    *models    = ecs_field(it, Walrus_Model, 2);

    for (i32 i = 0; i < it->count; ++i) {
        walrus_animator_tick(&animators[i], &models[i], it->delta_time);
    }
}

static void on_animator_model_add(ecs_iter_t *it)
{
    Walrus_Animator *animator = ecs_field(it, Walrus_Animator, 1);
    Walrus_Model    *model    = ecs_field(it, Walrus_Model, 2);

    walrus_animator_init(animator);
    walrus_animator_bind(animator, model);
    walrus_animator_play(animator, 0);
}

static void on_animator_remove(ecs_iter_t *it)
{
    Walrus_Animator *animator = ecs_field(it, Walrus_Animator, 1);
    walrus_animator_shutdown(animator);
}

void walrus_animator_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT(ecs, Walrus_Animator);
    ECS_COMPONENT(ecs, Walrus_Model);

    ECS_SYSTEM(ecs, animator_tick, EcsOnUpdate, Walrus_Animator, Walrus_Model);

    ECS_OBSERVER(ecs, on_animator_model_add, EcsOnSet, Walrus_Animator, Walrus_Model);
    ECS_OBSERVER(ecs, on_animator_remove, EcsOnRemove, Walrus_Animator);
}
