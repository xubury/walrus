#pragma once

#include <core/type.h>
#include <core/transform.h>

typedef struct {
    vec3 smooth_translation;
    vec3 translation;
    f32  smoothness;
} Walrus_FpsController;

void walrus_fps_controller_init(Walrus_FpsController *controller, f32 smoothness);

void walrus_fps_controller_shutdown(Walrus_FpsController *controller);

void walrus_fps_controller_tick(Walrus_FpsController *controller, Walrus_Transform *transform, f32 dt);
