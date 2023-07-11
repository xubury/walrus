#include <engine/systems/controller_system.h>
#include <engine/engine.h>

ECS_COMPONENT_DECLARE(Walrus_Controller);

static void controller_tick(ecs_iter_t *it)
{
    Walrus_Controller *controllers = ecs_field(it, Walrus_Controller, 1);
    Walrus_Transform  *transforms  = ecs_field(it, Walrus_Transform, 2);

    for (i32 i = 0; i < it->count; ++i) {
        Walrus_ControllerEvent event = {.entity     = it->entities[i],
                                        .transform  = &transforms[i],
                                        .delta_time = it->delta_time,
                                        .userdata   = controllers[i].userdata};
        controllers[i].tick(&event);
    }
}

static void on_fps_controller_remove(ecs_iter_t *it)
{
    Walrus_Controller *controller = ecs_field(it, Walrus_Controller, 1);

    controller->shutdown(controller->userdata);
}

void walrus_controller_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_Controller);

    ECS_SYSTEM(ecs, controller_tick, EcsOnUpdate, Walrus_Controller, Walrus_Transform);
    ECS_OBSERVER(ecs, on_fps_controller_remove, EcsOnRemove, Walrus_Controller, Walrus_Transform);
}
