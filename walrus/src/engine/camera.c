#include <engine/camera.h>
#include <core/math.h>

#include <cglm/cglm.h>

static bool update_view(Walrus_Camera *camera, Walrus_Transform const *transform)
{
    if (camera->need_update_view) {
        vec3 center;
        vec3 up;
        vec3 front;
        glm_quat_rotatev((f32 *)transform->rot, (vec3){0, 0, 1}, front);
        glm_quat_rotatev((f32 *)transform->rot, (vec3){0, 1, 0}, up);
        glm_vec3_sub((f32 *)transform->trans, front, center);
        glm_lookat((f32 *)transform->trans, center, up, camera->view);
        camera->need_update_view = false;
        return true;
    }
    return false;
}

static bool update_projection(Walrus_Camera *camera)
{
    if (camera->need_update_projection) {
        glm_perspective(camera->fov, camera->aspect, camera->near_z, camera->far_z, camera->projection);
        camera->need_update_projection = false;
        return true;
    }
    return false;
}

void walrus_camera_init(Walrus_Camera *camera, vec3 const pos, versor const rot, f32 fov, f32 aspect, f32 near_z,
                        f32 far_z)
{
    Walrus_Transform transform;
    glm_vec3_copy((f32 *)pos, transform.trans);
    glm_quat_copy((f32 *)rot, transform.rot);
    glm_vec3_one(transform.scale);

    camera->fov    = fov;
    camera->aspect = aspect;
    camera->near_z = near_z;
    camera->far_z  = far_z;

    camera->need_update_view       = true;
    camera->need_update_projection = true;

    walrus_camera_update(camera, &transform);
}

void walrus_camera_update(Walrus_Camera *camera, Walrus_Transform const *transform)
{
    bool change = update_view(camera, transform);
    change |= update_projection(camera);
    if (change) {
        glm_mat4_mul(camera->projection, camera->view, camera->vp);
    }
}

bool walrus_camera_frustum_cull_test(Walrus_Camera const *camera, mat4 const world, vec3 const min, vec3 const max)
{
    vec4 corners[8] = {
        {min[0], min[1], min[2], 1.0}, {max[0], min[1], min[2], 1.0},
        {min[0], max[1], min[2], 1.0}, {max[0], max[1], min[2], 1.0},

        {min[0], min[1], max[2], 1.0}, {max[0], min[1], max[2], 1.0},
        {min[0], max[1], max[2], 1.0}, {max[0], max[1], max[2], 1.0},
    };
    mat4 mvp;
    glm_mat4_mul((vec4 *)camera->vp, (vec4 *)world, mvp);

    bool inside = false;

    // FIXME: incorrect implementaion, frustum may be inside AABB
    for (u8 i = 0; i < 8; ++i) {
        vec4 corner;
        glm_mat4_mulv(mvp, corners[i], corner);
        if ((corner[0] <= corner[3] && corner[0] >= -corner[3]) &&
            (corner[1] <= corner[3] && corner[1] >= -corner[3]) && (corner[2] <= corner[3] && corner[2] >= 0)) {
            inside = true;
            break;
        }
    }

    inside = true;

    return inside;
}
