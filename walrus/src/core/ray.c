#include <core/ray.h>
#include <core/math.h>
#include <cglm/cglm.h>

bool walrus_intersect_ray_plane(vec3 world, vec3 const origin, vec3 const direction, vec3 const normal, vec3 const p)
{
    f32 numer = glm_dot((float *)normal, (float *)origin) - glm_dot((float *)normal, (float *)p);
    f32 denom = glm_dot((float *)normal, (float *)direction);

    // orthogonal, can't intercect
    if (walrus_abs(denom) < FLT_EPSILON) {
        return false;
    }

    glm_vec3_scale((float *)direction, -numer / denom, world);
    glm_vec3_add(world, (float *)origin, world);
    return true;
}
