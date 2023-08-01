#pragma once

#include <core/type.h>
#include <core/transform.h>
#include <engine/controller.h>

typedef struct {
    vec3 smooth_translation;

    vec2 smooth_rotation;

    f32  speed;
    vec2 rotate_speed;
    f32  smoothness;

    mat3 ground_transform;
} Walrus_FpsController;

POLY_DECLARE_DERIVED(Walrus_Controller, Walrus_FpsController, walrus_fps_controller)
