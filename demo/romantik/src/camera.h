#pragma once

#include <core/type.h>
#include <cglm/cglm.h>

typedef struct {
    vec3  movement;
    f32   speed;
    f32   angle_movement;
    f32   angle_speed;
    f32   smoothness;
    vec3  focus_pos;
    float view_len;
    f32   view_angle;
    f32   view_yangle;
    mat4  view;
} CameraData;

void camera_init(CameraData *cam);

void camera_tick(CameraData *cam, f32 dt);
