#pragma once

#include <core/type.h>
#include <core/transform.h>
#include <engine/geometry.h>

typedef struct {
    Walrus_Plane left;
    Walrus_Plane right;
    Walrus_Plane bottom;
    Walrus_Plane top;
    Walrus_Plane near;
    Walrus_Plane far;
} Walrus_Frustrum;

typedef struct {
    f32 fov;
    f32 aspect;
    f32 near_z;
    f32 far_z;

    mat4 view;
    mat4 projection;
    mat4 vp;
    bool need_update_view;
    bool need_update_projection;
} Walrus_Camera;

void walrus_camera_init(Walrus_Camera *camera, vec3 const pos, versor const rot, f32 fov, f32 aspect, f32 near_z,
                        f32 far_z);

void walrus_camera_update(Walrus_Camera *camera, Walrus_Transform const *transform);

bool walrus_camera_frustum_cull_test(Walrus_Camera const *camera, mat4 const world, vec3 const min, vec3 const max);
