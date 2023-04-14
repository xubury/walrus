#include "camera.h"

#include <core/math.h>
#include <engine/engine.h>
#include <rhi/rhi.h>

static void camera_get_dir(CameraData *cam, vec3 dir)
{
    glm_vec3_copy((vec3){cos(cam->view_angle), sin(cam->view_yangle), sin(cam->view_angle)}, dir);
}

static void camera_get_eye(CameraData *cam, vec3 eye)
{
    vec3 dir;
    camera_get_dir(cam, dir);
    glm_vec3_scale(dir, cam->view_len, dir);
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
        glm_vec3_scale_as(step, cam->speed * dt, step);
    }

    f32 angle_step = 0.f;
    if (walrus_input_down(input->mouse, WR_MOUSE_BTN_RIGHT)) {
        vec2 axis;
        walrus_input_relaxis(input->mouse, WR_MOUSE_AXIS_CURSOR, axis, 2);
        if (axis[0] > FLT_EPSILON) {
            angle_step = 1;
        }
        else if (axis[0] < -FLT_EPSILON) {
            angle_step = -1;
        }
        angle_step = cam->angle_speed * angle_step * dt;
    }

    bool const update_view =
        glm_vec3_norm2(cam->movement) >= FLT_EPSILON || walrus_abs(cam->angle_movement) >= FLT_EPSILON;

    glm_vec3_lerp(cam->movement, step, walrus_clamp(cam->smoothness * dt, 0.0, 1.0), cam->movement);
    cam->angle_movement = glm_lerp(cam->angle_movement, angle_step, walrus_clamp(cam->smoothness * dt, 0.0, 1.0));

    glm_vec3_add(cam->focus_pos, cam->movement, cam->focus_pos);
    cam->view_angle += cam->angle_movement;
    if (cam->view_angle > M_PI * 2) cam->view_angle -= M_PI * 2;

    if (update_view) {
        camera_update_view(cam);
    }
}

void camera_init(CameraData *cam)
{
    glm_vec3_copy((vec3){0, 0, 0}, cam->focus_pos);
    glm_vec3_copy((vec3){0, 0, 0}, cam->movement);
    cam->smoothness     = 10.0f;
    cam->speed          = 10.0f;
    cam->angle_speed    = 2.0f;
    cam->angle_movement = 0.0f;
    cam->view_len       = 5.f;
    cam->view_angle     = glm_rad(45);
    cam->view_yangle    = glm_rad(70);
    camera_update_view(cam);
}
