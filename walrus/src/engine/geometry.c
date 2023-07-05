#include <engine/geometry.h>
#include <core/math.h>

f32 walrus_plane_p_dist(Walrus_Plane const *plane, vec3 const p)
{
    return glm_dot((f32 *)plane->normal, (f32 *)p) - plane->dist;
}

void walrus_bounding_box_from_min_max(Walrus_BoundingBox *box, vec3 const min, vec3 const max)
{
    glm_vec3_add((f32 *)min, (f32 *)max, box->center);
    glm_vec3_scale(box->center, 0.5, box->center);
    glm_vec3_sub((f32 *)max, box->center, box->extends);
}

bool walrus_bounding_box_intersects_plane(Walrus_BoundingBox const *box, Walrus_Plane const *plane)
{
    f32 const r = box->extends[0] * walrus_abs(plane->normal[0]) + box->extends[1] * walrus_abs(plane->normal[1]) +
                  box->extends[2] * walrus_abs(plane->normal[2]);

    return -r <= walrus_plane_p_dist(plane, box->center);
}
