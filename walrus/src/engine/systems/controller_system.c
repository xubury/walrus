#include <engine/systems/controller_system.h>
#include <core/memory.h>
#include <core/macro.h>

ECS_COMPONENT_DECLARE(Walrus_Controller);

static void controller_tick(ecs_iter_t *it)
{
    Walrus_Controller *controllers = ecs_field(it, Walrus_Controller, 1);
    Walrus_Transform  *transforms  = ecs_field(it, Walrus_Transform, 2);

    for (i32 i = 0; i < it->count; ++i) {
        Walrus_ControllerEvent event = {
            .entity     = it->entities[i],
            .transform  = &transforms[i],
            .delta_time = it->delta_time,
        };
        POLY_FUNC(&controllers[i], controller_tick)(&controllers[i], &event);
    }
}

static void on_controller_add(ecs_iter_t *it)
{
    Walrus_Controller *controller = ecs_field(it, Walrus_Controller, 1);

    walrus_input_map_init(&controller->map);
    if (POLY_FUNC(controller, controller_init)) {
        POLY_FUNC(controller, controller_init)(controller);
    }
}

static void on_controller_remove(ecs_iter_t *it)
{
    Walrus_Controller *controller = ecs_field(it, Walrus_Controller, 1);

    if (POLY_FUNC(controller, controller_shutdown)) {
        POLY_FUNC(controller, controller_shutdown)(controller);
    }
    walrus_input_map_shutdown(&controller->map);
    poly_free(controller);
}

static void controller_system_init(Walrus_System *sys)
{
    ecs_world_t *ecs = sys->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_Controller);

    ECS_SYSTEM(ecs, controller_tick, EcsOnUpdate, Walrus_Controller, Walrus_Transform);
    ECS_OBSERVER(ecs, on_controller_add, EcsOnSet, Walrus_Controller, Walrus_Transform);
    ECS_OBSERVER(ecs, on_controller_remove, EcsOnRemove, Walrus_Controller);
}

POLY_DEFINE_DERIVED(Walrus_System, ControllerSystem, controller_system_create,
                    POLY_IMPL(on_system_init, controller_system_init))
