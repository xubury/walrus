#include <engine/camera.h>
#include <core/math.h>

#include <cglm/cglm.h>

static void update_view(Walrus_Camera *camera)
{
    if (camera->need_update_view) {
        vec3 center;
        vec3 up;
        vec3 front;
        glm_quat_rotatev(camera->transform.rot, (vec3){0, 0, 1}, front);
        glm_quat_rotatev(camera->transform.rot, (vec3){0, 1, 0}, up);
        glm_vec3_sub(camera->transform.trans, front, center);
        glm_lookat(camera->transform.trans, center, up, camera->view);
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

void walrus_camera_init(Walrus_Camera *camera, vec3 pos, versor rot, f32 fov, f32 aspect, f32 near_z, f32 far_z)
{
    glm_vec3_copy(pos, camera->transform.trans);
    glm_quat_copy(rot, camera->transform.rot);
    glm_vec3_one(camera->transform.scale);

    camera->fov    = fov;
    camera->aspect = aspect;
    camera->near_z = near_z;
    camera->far_z  = far_z;

    walrus_camera_mark_dirty(camera);
    walrus_camera_update(camera);
}

void walrus_camera_mark_dirty(Walrus_Camera *camera)
{
    camera->need_update_view       = true;
    camera->need_update_projection = true;
}

void walrus_camera_update(Walrus_Camera *camera)
{
    update_view(camera);
    update_projection(camera);
}
