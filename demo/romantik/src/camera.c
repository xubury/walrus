#include "camera.h"

#include <core/math.h>
#include <engine/engine.h>

static void camera_update_view(CameraData *cam, mat4 view)
{
    vec3 dir = {-cos(cam->view_angle), -sin(cam->view_yangle), -sin(cam->view_angle)};
    vec3 up  = {0, 1, 0};
    vec3 center;
    glm_vec3_add(cam->pos, dir, center);
    glm_lookat(cam->pos, center, up, view);
}

void camera_tick(CameraData *cam, f32 dt)
{
    Walrus_Input *input = walrus_engine_input();
    bool          move  = false;
    vec3          step  = {0, 0, 0};
    if (walrus_input_down(input->keyboard, WR_KEY_W)) {
        move = true;
        glm_vec3_add(step, (vec3){0, 0, -1}, step);
    }
    if (walrus_input_down(input->keyboard, WR_KEY_S)) {
        move = true;
        glm_vec3_add(step, (vec3){0, 0, 1}, step);
    }
    if (walrus_input_down(input->keyboard, WR_KEY_A)) {
        move = true;
        glm_vec3_add(step, (vec3){-1, 0, 0}, step);
    }
    if (walrus_input_down(input->keyboard, WR_KEY_D)) {
        move = true;
        glm_vec3_add(step, (vec3){1, 0, 0}, step);
    }
    if (move) {
        glm_vec3_rotate(step, cam->view_angle, (vec3){0, 1, 0});
        glm_vec3_scale_as(step, cam->speed * dt, step);
    }
    bool const update_view = glm_vec3_norm2(cam->movement) >= FLT_EPSILON;
    glm_vec3_lerp(cam->movement, step, walrus_clamp(cam->smoothness * dt, 0.0, 1.0), cam->movement);
    glm_vec3_add(cam->pos, cam->movement, cam->pos);
    if (update_view) {
        camera_update_view(cam, cam->view);
    }
}

void camera_init(CameraData *cam)
{
    glm_vec3_copy((vec3){1, 3, 1}, cam->pos);
    glm_vec3_copy((vec3){0, 0, 0}, cam->movement);
    cam->len         = 3.0f;
    cam->smoothness  = 10.0f;
    cam->speed       = 10.0f;
    cam->view_angle  = glm_rad(45);
    cam->view_yangle = glm_rad(60);
}
