#pragma once

#include <flecs.h>

#include <core/transform.h>
#include <engine/animator.h>
#include <engine/renderer.h>
#include <engine/camera.h>
#include <engine/controller.h>

extern ECS_COMPONENT_DECLARE(Walrus_Animator);
extern ECS_COMPONENT_DECLARE(Walrus_Camera);
extern ECS_COMPONENT_DECLARE(Walrus_Controller);
extern ECS_COMPONENT_DECLARE(Walrus_Transform);

typedef Walrus_Transform Walrus_LocalTransform;
extern ECS_COMPONENT_DECLARE(Walrus_LocalTransform);

extern ECS_COMPONENT_DECLARE(Walrus_Renderer);
