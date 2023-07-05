#pragma once

#include <core/type.h>
#include <cglm/cglm.h>

typedef struct {
    vec3 center;
    vec3 extends;
} Walrus_BoundingBox;

typedef struct {
    f32  dist;
    vec3 normal;
} Walrus_Plane;

typedef struct {
    Walrus_Plane left;
    Walrus_Plane right;
    Walrus_Plane bottom;
    Walrus_Plane top;
    Walrus_Plane near;
    Walrus_Plane far;
} Walrus_Frustum;

f32 walrus_plane_p_dist(Walrus_Plane const *plane, vec3 const p);

void walrus_plane_from_pn(Walrus_Plane *plane, vec3 const p, vec3 const normal);

void walrus_bounding_box_from_min_max(Walrus_BoundingBox *box, vec3 const min, vec3 const max);

bool walrus_bounding_box_intersect_or_front_plane(Walrus_BoundingBox const *box, Walrus_Plane const *plane);

bool walrus_bounding_box_intersects_frustum(Walrus_BoundingBox const *box, Walrus_Frustum const *frustum);

void walrus_bounding_box_transform(Walrus_BoundingBox *box, mat4 const transform);
