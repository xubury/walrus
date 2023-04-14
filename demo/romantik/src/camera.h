#pragma once

#include <core/type.h>
#include <cglm/cglm.h>

typedef struct {
    vec3 movement;
    f32  angle_movement;
    vec3 focus_pos;
    f32  move_speed;
    f32  rot_speed;

    f32 arm_len;
    f32 arm_movement;
    f32 view_angle;
    f32 view_yangle;

    mat4 view;

    f32 smoothness;
} CameraData;

void camera_init(CameraData *cam);

void camera_tick(CameraData *cam, f32 dt);
