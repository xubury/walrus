#pragma once

#include <flecs.h>
#include <engine/fps_controller.h>

extern ECS_COMPONENT_DECLARE(Walrus_FpsController);

void walrus_controller_system_init(void);
