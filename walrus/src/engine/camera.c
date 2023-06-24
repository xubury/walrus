#include <engine/camera.h>
#include <core/math.h>

#include <cglm/cglm.h>

static void update_view(Walrus_Camera *camera, Walrus_Transform const *transform)
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
    }
}

static void update_projection(Walrus_Camera *camera)
{
    if (camera->need_update_projection) {
        glm_perspective(camera->fov, camera->aspect, camera->near_z, camera->far_z, camera->projection);
        camera->need_update_projection = false;
    }
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

    walrus_camera_mark_dirty(camera);
    walrus_camera_update(camera, &transform);
}

void walrus_camera_mark_dirty(Walrus_Camera *camera)
{
    camera->need_update_view       = true;
    camera->need_update_projection = true;
}

void walrus_camera_update(Walrus_Camera *camera, Walrus_Transform const *transform)
{
    update_view(camera, transform);
    update_projection(camera);
}

bool walrus_camera_cull_test(Walrus_Camera *camera, Walrus_BoundingBox *box)
{
    walrus_unused(camera);
    walrus_unused(box);

    // TODO:

    return true;
}
