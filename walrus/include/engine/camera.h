#pragma once

#include <core/type.h>
#include <core/transform.h>

typedef struct {
    f32 fov;
    f32 aspect;
    f32 near_z;
    f32 far_z;

    mat4 view;
    mat4 projection;
    bool need_update_view;
    bool need_update_projection;
} Walrus_Camera;

typedef struct {
    vec3 min;
    vec3 max;
} Walrus_BoundingBox;

void walrus_camera_init(Walrus_Camera *camera, vec3 const pos, versor const rot, f32 fov, f32 aspect, f32 near_z,
                        f32 far_z);

void walrus_camera_mark_dirty(Walrus_Camera *camera);

void walrus_camera_update(Walrus_Camera *camera, Walrus_Transform const *transform);

bool walrus_camera_cull_test(Walrus_Camera *camera, Walrus_BoundingBox *box);
