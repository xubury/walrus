#pragma once

#include <core/type.h>
#include <core/transform.h>
#include <engine/controller.h>
#include <engine/input_map.h>

typedef struct {
    vec3 smooth_translation;

    vec2 smooth_rotation;

    f32  speed;
    vec2 rotate_speed;
    f32  smoothness;

    mat3 ground_transform;
} Walrus_FpsController;

void walrus_fps_controller_init(Walrus_Controller *controller);

void walrus_fps_controller_tick(Walrus_ControllerEvent *event);

void walrus_fps_controller_shutdown(Walrus_Controller *controller);
