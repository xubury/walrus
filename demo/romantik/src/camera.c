#include "camera.h"

#include <core/math.h>
#include <engine/engine.h>
#include <rhi/rhi.h>

static void camera_get_dir(CameraData *cam, vec3 dir)
{
    glm_vec3_copy((vec3){cos(cam->view_angle), cos(cam->view_yangle), sin(cam->view_angle)}, dir);
    glm_vec3_normalize(dir);
}

static void camera_get_eye(CameraData *cam, vec3 eye)
{
    vec3 dir;
    camera_get_dir(cam, dir);
    glm_vec3_scale(dir, cam->arm_len, dir);
    glm_vec3_add(cam->focus_pos, dir, eye);
}

static void camera_update_view(CameraData *cam)
{
    vec3 up = {0, 1, 0};
    vec3 eye;
    camera_get_eye(cam, eye);
    glm_lookat(eye, cam->focus_pos, up, cam->view);
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
        glm_vec3_rotate(step, M_PI * 0.5 - cam->view_angle, (vec3){0, 1, 0});
        glm_vec3_scale_as(step, cam->move_speed * dt, step);
    }

    f32 angle_step = 0.f;
    if (walrus_input_down(input->mouse, WR_MOUSE_BTN_RIGHT)) {
        f32 x_offset;
        walrus_input_relaxis(input->mouse, WR_MOUSE_AXIS_CURSOR, &x_offset, NULL, NULL);
        if (x_offset > FLT_EPSILON) {
            angle_step = cam->rot_speed * dt;
        }
        else if (x_offset < -FLT_EPSILON) {
            angle_step = -cam->rot_speed * dt;
        }
    }

    float arm_step = 0.f;
    float y_scroll = 0.f;
    walrus_input_relaxis(input->mouse, WR_MOUSE_AXIS_WHEEL, NULL, &y_scroll, NULL);
    if (y_scroll > FLT_EPSILON) {
        arm_step = -cam->move_speed * 2.0 * dt;
    }
    else if (y_scroll < -FLT_EPSILON) {
        arm_step = cam->move_speed * 2.0 * dt;
    }

    glm_vec3_lerp(cam->movement, step, walrus_clamp(cam->smoothness * dt, 0.0, 1.0), cam->movement);
    cam->angle_movement = glm_lerp(cam->angle_movement, angle_step, walrus_clamp(cam->smoothness * dt, 0.0, 1.0));
    cam->arm_movement   = glm_lerp(cam->arm_movement, arm_step, walrus_clamp(cam->smoothness * dt, 0.0, 1.0));

    glm_vec3_add(cam->focus_pos, cam->movement, cam->focus_pos);

    cam->view_angle += cam->angle_movement;
    if (cam->view_angle > M_PI * 2) cam->view_angle -= M_PI * 2;

    cam->arm_len = walrus_max(cam->arm_len + cam->arm_movement, 0.1f);

    bool const update_view = glm_vec3_norm2(cam->movement) >= FLT_EPSILON ||
                             walrus_abs(cam->angle_movement) >= FLT_EPSILON ||
                             walrus_abs(cam->arm_movement) >= FLT_EPSILON;

    if (update_view) {
        camera_update_view(cam);
    }
}

void camera_init(CameraData *cam)
{
    glm_vec3_copy((vec3){0, 0, 0}, cam->focus_pos);
    glm_vec3_copy((vec3){0, 0, 0}, cam->movement);
    cam->smoothness = 10.0f;

    cam->move_speed = 10.0f;
    cam->rot_speed  = 8.0f;

    cam->angle_movement = 0.0f;
    cam->arm_len        = 5.f;
    cam->arm_movement   = 0.f;
    cam->view_angle     = glm_rad(45);
    cam->view_yangle    = glm_rad(30);
    camera_update_view(cam);
}
