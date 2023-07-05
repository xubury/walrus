#include <engine/geometry.h>
#include <core/math.h>

f32 walrus_plane_p_dist(Walrus_Plane const *plane, vec3 const p)
{
    return glm_dot((f32 *)plane->normal, (f32 *)p) - plane->dist;
}

void walrus_plane_from_pn(Walrus_Plane *plane, vec3 const p, vec3 const normal)
{
    glm_vec3_copy((f32 *)normal, plane->normal);
    plane->dist = glm_dot((f32 *)normal, (f32 *)p);
}

void walrus_bounding_box_from_min_max(Walrus_BoundingBox *box, vec3 const min, vec3 const max)
{
    glm_vec3_add((f32 *)min, (f32 *)max, box->center);
    glm_vec3_scale(box->center, 0.5, box->center);
    glm_vec3_sub((f32 *)max, box->center, box->extends);
}

bool walrus_bounding_box_intersect_or_front_plane(Walrus_BoundingBox const *box, Walrus_Plane const *plane)
{
    f32 const r = box->extends[0] * walrus_abs(plane->normal[0]) + box->extends[1] * walrus_abs(plane->normal[1]) +
                  box->extends[2] * walrus_abs(plane->normal[2]);

    return -r <= walrus_plane_p_dist(plane, box->center);
}

bool walrus_bounding_box_intersects_frustum(Walrus_BoundingBox const *box, Walrus_Frustum const *frustum)
{
    return walrus_bounding_box_intersect_or_front_plane(box, &frustum->left) &&
           walrus_bounding_box_intersect_or_front_plane(box, &frustum->right) &&
           walrus_bounding_box_intersect_or_front_plane(box, &frustum->top) &&
           walrus_bounding_box_intersect_or_front_plane(box, &frustum->bottom) &&
           walrus_bounding_box_intersect_or_front_plane(box, &frustum->near) &&
           walrus_bounding_box_intersect_or_front_plane(box, &frustum->far);
}

void walrus_bounding_box_transform(Walrus_BoundingBox *box, mat4 const transform)
{
    glm_mat4_mulv3((vec4 *)transform, box->center, 1.0, box->center);
    vec3 extends = {
        fabs(box->extends[0] * transform[0][0]) + fabs(box->extends[1] * transform[1][0]) +
            fabs(box->extends[2] * transform[2][0]),
        fabs(box->extends[0] * transform[0][1]) + fabs(box->extends[1] * transform[1][1]) +
            fabs(box->extends[2] * transform[2][1]),
        fabs(box->extends[0] * transform[0][2]) + fabs(box->extends[1] * transform[1][2]) +
            fabs(box->extends[2] * transform[2][2]),
    };
    glm_vec3_copy(extends, box->extends);
}
