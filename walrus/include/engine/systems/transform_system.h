#pragma once

#include <core/transform.h>

#include <flecs.h>

typedef Walrus_Transform Walrus_LocalTransform;

extern ECS_COMPONENT_DECLARE(Walrus_Transform);
extern ECS_COMPONENT_DECLARE(Walrus_LocalTransform);

void walrus_transform_system_init(void);
