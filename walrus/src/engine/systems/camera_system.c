#include <engine/systems/camera_system.h>
#include <engine/camera.h>
#include <engine/engine.h>

void camera_tick(ecs_iter_t *it)
{
    Walrus_Camera    *cameras    = ecs_field(it, Walrus_Camera, 1);
    Walrus_Transform *transforms = ecs_field(it, Walrus_Transform, 2);

    for (i32 i = 0; i < it->count; ++i) {
        walrus_camera_mark_dirty(&cameras[i]);
        walrus_camera_update(&cameras[i], &transforms[i]);
    }
}

void camera_on_add(ecs_iter_t *it)
{
    Walrus_Camera    *camera    = ecs_field(it, Walrus_Camera, 1);
    Walrus_Transform *transform = ecs_field(it, Walrus_Transform, 2);
    walrus_camera_mark_dirty(camera);
    walrus_camera_update(camera, transform);
}

void walrus_camera_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT(ecs, Walrus_Camera);
    ECS_COMPONENT(ecs, Walrus_Transform);

    ECS_SYSTEM(ecs, camera_tick, EcsOnUpdate, Walrus_Camera, Walrus_Transform);
    ECS_OBSERVER(ecs, camera_on_add, EcsOnSet, Walrus_Camera, Walrus_Transform);
}
