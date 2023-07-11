#include <engine/systems/camera_system.h>
#include <engine/camera.h>
#include <engine/engine.h>

ECS_COMPONENT_DECLARE(Walrus_Camera);

static void camera_tick(ecs_iter_t *it)
{
    Walrus_Camera    *cameras    = ecs_field(it, Walrus_Camera, 1);
    Walrus_Transform *transforms = ecs_field(it, Walrus_Transform, 2);

    for (i32 i = 0; i < it->count; ++i) {
        cameras[i].need_update_view = true;
        walrus_camera_update(&cameras[i], &transforms[i]);
    }
}

void camera_on_add(ecs_iter_t *it)
{
    Walrus_Camera    *camera    = ecs_field(it, Walrus_Camera, 1);
    Walrus_Transform *transform = ecs_field(it, Walrus_Transform, 2);

    walrus_camera_init(camera, transform->trans, transform->rot, camera->fov, camera->aspect, camera->near_z,
                       camera->far_z);
}

void walrus_camera_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_Camera);

    ECS_SYSTEM(ecs, camera_tick, EcsOnUpdate, Walrus_Camera, Walrus_Transform);
    ECS_OBSERVER(ecs, camera_on_add, EcsOnSet, Walrus_Camera, Walrus_Transform);
}
