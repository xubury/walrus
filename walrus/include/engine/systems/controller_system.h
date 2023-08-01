#pragma once

#include <engine/controller.h>
#include <engine/system.h>

#include <flecs.h>

typedef void* ControllerSystem;

POLY_DECLARE_DERIVED(Walrus_System, ControllerSystem, controller_system_create)
