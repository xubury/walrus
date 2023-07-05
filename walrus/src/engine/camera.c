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
    Walrus_Frustum frustum;
    walrus_frustum_from_camera_local(camera, &frustum);

    Walrus_BoundingBox box;
    walrus_bounding_box_from_min_max(&box, min, max);

    mat4 t;
    glm_mat4_mul((vec4 *)camera->view, (vec4 *)world, t);
    walrus_bounding_box_transform(&box, t);

    return walrus_bounding_box_intersects_frustum(&box, &frustum);
}

static void frustrum_construct(Walrus_Frustum *frustum, vec3 const cam_right, vec3 const cam_up, vec3 const cam_front,
                               vec3 cam_pos, f32 fov, f32 aspect, f32 near_z, f32 far_z)
{
    f32 const half_vside = far_z * tanf(fov * 0.5f);
    f32 const half_hside = half_vside * aspect;

    vec3 near, left, right, top, bottom;
    glm_vec3_scale((f32 *)cam_front, -1, near);

    vec3 near_mult;
    glm_vec3_scale(near, far_z, near_mult);

    glm_vec3_scale((f32 *)cam_right, half_hside, right);
    glm_vec3_sub(near_mult, right, right);
    glm_cross(right, (f32 *)cam_up, right);

    glm_vec3_scale((f32 *)cam_right, half_hside, left);
    glm_vec3_add(near_mult, left, left);
    glm_cross((f32 *)cam_up, left, left);

    glm_vec3_scale((f32 *)cam_up, half_vside, top);
    glm_vec3_sub(near_mult, top, top);
    glm_cross((f32 *)cam_right, top, top);

    glm_vec3_scale((f32 *)cam_up, half_vside, bottom);
    glm_vec3_add(near_mult, bottom, bottom);
    glm_cross(bottom, (f32 *)cam_right, bottom);

    vec3 p_near;
    glm_vec3_scale((f32 *)cam_front, near_z, p_near);
    glm_vec3_add(cam_pos, p_near, p_near);

    vec3 p_far;
    glm_vec3_add(cam_pos, near_mult, p_far);

    walrus_plane_from_pn(&frustum->near, p_near, near);
    walrus_plane_from_pn(&frustum->far, p_far, cam_front);
    walrus_plane_from_pn(&frustum->left, cam_pos, left);
    walrus_plane_from_pn(&frustum->right, cam_pos, right);
    walrus_plane_from_pn(&frustum->top, cam_pos, top);
    walrus_plane_from_pn(&frustum->bottom, cam_pos, bottom);
}

void walrus_frustum_from_camera_local(Walrus_Camera const *camera, Walrus_Frustum *frustum)
{
    frustrum_construct(frustum, (vec3){1, 0, 0}, (vec3){0, 1, 0}, (vec3){0, 0, 1}, (vec3){0, 0, 0}, camera->fov,
                       camera->aspect, camera->near_z, camera->far_z);
}

void walrus_frustum_from_camera(Walrus_Camera const *camera, Walrus_Frustum *frustum)
{
    mat4 m;
    glm_mat4_copy((vec4 *)camera->view, m);
    glm_inv_tr(m);
    Walrus_Transform cam_world;
    walrus_transform_decompose(&cam_world, m);

    vec3 cam_front, cam_right, cam_up;
    walrus_transform_front(&cam_world, cam_front);
    walrus_transform_up(&cam_world, cam_up);
    walrus_transform_right(&cam_world, cam_right);

    frustrum_construct(frustum, cam_right, cam_up, cam_front, cam_world.trans, camera->fov, camera->aspect,
                       camera->near_z, camera->far_z);
}
