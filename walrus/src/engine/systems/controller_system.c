#include <engine/systems/controller_system.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/log.h>

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

static void on_controller_set(ecs_iter_t *it)
{
    Walrus_Controller *ctrls = ecs_field(it, Walrus_Controller, 1);

    for (i32 i = 0; i < it->count; ++i) {
        walrus_input_map_init(&ctrls[i].map);
        if (POLY_FUNC(&ctrls[i], controller_init)) {
            POLY_FUNC(&ctrls[i], controller_init)(&ctrls[i]);
        }
    }
}

static void on_controller_unset(ecs_iter_t *it)
{
    Walrus_Controller *ctrls = ecs_field(it, Walrus_Controller, 1);

    for (i32 i = 0; i < it->count; ++i) {
        if (POLY_FUNC(&ctrls[i], controller_shutdown)) {
            POLY_FUNC(&ctrls[i], controller_shutdown)(&ctrls[i]);
        }
        walrus_input_map_shutdown(&ctrls[i].map);
        poly_free(&ctrls[i]);
    }
}

static void controller_system_init(Walrus_System *sys)
{
    ecs_world_t *ecs = sys->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_Controller);

    ECS_SYSTEM(ecs, controller_tick, EcsOnUpdate, Walrus_Controller, Walrus_Transform);

    ECS_OBSERVER(ecs, on_controller_set, EcsOnSet, Walrus_Controller, Walrus_Transform);
    ECS_OBSERVER(ecs, on_controller_unset, EcsUnSet, Walrus_Controller, Walrus_Transform);
}

POLY_DEFINE_DERIVED(Walrus_System, void, controller_system_create, POLY_IMPL(on_system_init, controller_system_init))
