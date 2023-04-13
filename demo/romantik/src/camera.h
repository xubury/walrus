#pragma once

#include <core/type.h>
#include <cglm/cglm.h>

typedef struct {
    vec3 pos;
    vec3 movement;
    f32  speed;
    f32  smoothness;
    f32  len;
    f32  view_angle;
    f32  view_yangle;
    mat4 view;
} CameraData;

void camera_init(CameraData *cam);

void camera_tick(CameraData *cam, f32 dt);
