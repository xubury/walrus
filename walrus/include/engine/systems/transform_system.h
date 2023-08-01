#pragma once

#include <core/transform.h>
#include <engine/system.h>

#include <flecs.h>

typedef Walrus_Transform Walrus_LocalTransform;

extern ECS_COMPONENT_DECLARE(Walrus_LocalTransform);

typedef void* TransformSystem;

POLY_DECLARE_DERIVED(Walrus_System, TransformSystem, transform_system_create)
