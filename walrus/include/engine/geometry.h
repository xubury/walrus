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

f32 walrus_plane_p_dist(Walrus_Plane const *plane, vec3 const p);

void walrus_bounding_box_from_min_max(Walrus_BoundingBox *box, vec3 const min, vec3 const max);

bool walrus_bounding_box_intersects_plane(Walrus_BoundingBox const *box, Walrus_Plane const *plane);
