#include <engine/systems/transform_system.h>
#include <engine/engine.h>

ECS_COMPONENT_DECLARE(Walrus_Transform);

void walrus_transform_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_Transform);
}
