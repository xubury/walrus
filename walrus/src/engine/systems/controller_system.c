#include <engine/systems/controller_system.h>
#include <engine/fps_controller.h>
#include <engine/engine.h>
#include <core/log.h>
#include <core/math.h>

static void controller_tick(ecs_iter_t *it)
{
    Walrus_FpsController *controllers = ecs_field(it, Walrus_FpsController, 1);
    Walrus_Transform     *transforms  = ecs_field(it, Walrus_Transform, 2);

    f32 const dt = it->delta_time;

    for (i32 i = 0; i < it->count; ++i) {
        walrus_fps_controller_tick(&controllers[i], &transforms[i], dt);
    }
}

static void on_fps_controller_add(ecs_iter_t *it)
{
    Walrus_FpsController *controller = ecs_field(it, Walrus_FpsController, 1);

    walrus_fps_controller_init(controller, controller->speed, controller->rotate_speed, controller->smoothness);
}

static void on_fps_controller_remove(ecs_iter_t *it)
{
    Walrus_FpsController *controller = ecs_field(it, Walrus_FpsController, 1);

    walrus_fps_controller_shutdown(controller);
}

void walrus_controller_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT(ecs, Walrus_FpsController);
    ECS_COMPONENT(ecs, Walrus_Transform);

    ECS_SYSTEM(ecs, controller_tick, EcsOnUpdate, Walrus_FpsController, Walrus_Transform);
    ECS_OBSERVER(ecs, on_fps_controller_add, EcsOnSet, Walrus_FpsController, Walrus_Transform);
    ECS_OBSERVER(ecs, on_fps_controller_remove, EcsOnRemove, Walrus_FpsController, Walrus_Transform);
}
