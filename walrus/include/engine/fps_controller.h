#pragma once

#include <core/type.h>
#include <core/transform.h>
#include <engine/controller.h>
#include <engine/control_map.h>

typedef struct {
    vec3 smooth_translation;
    vec3 translation;

    vec2 smooth_rotation;
    vec2 rotation;

    f32  speed;
    vec2 rotate_speed;
    f32  smoothness;

    mat3 ground_transform;

    Walrus_ControlMap map;
} Walrus_FpsController;

void walrus_fps_controller_init(Walrus_FpsController *controller, f32 speed, vec2 rotation_speed, f32 smoothness);

void walrus_fps_controller_shutdown(Walrus_FpsController *controller);

void walrus_fps_controller_tick(Walrus_ControllerEvent *event);
