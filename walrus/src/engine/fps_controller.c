#include <engine/fps_controller.h>
#include <engine/engine.h>
#include <core/math.h>

static void fps_movement(vec3 scale, void *userdata)
{
    Walrus_FpsController *controller = userdata;

    vec3 tmp;
    glm_vec3_mul(scale, (vec3){controller->speed, controller->speed, controller->speed}, tmp);
    glm_vec3_add(controller->translation, tmp, controller->translation);
}

static void fps_rotation(vec3 scale, void *userdata)
{
    Walrus_FpsController *controller = userdata;

    vec2 tmp;
    glm_vec2_mul(scale, controller->rotate_speed, tmp);
    glm_vec2_add(controller->rotation, tmp, controller->rotation);
    glm_vec2_clamp(controller->rotation, -180, 180);
}

void walrus_fps_controller_init(Walrus_FpsController *controller, f32 speed, vec2 rotation_speed, f32 smoothness)
{
    walrus_control_map_init(&controller->map);
    walrus_control_bind_axis(&controller->map, "FpsMovement", fps_movement);
    walrus_control_bind_axis(&controller->map, "FpsRotation", fps_rotation);
    walrus_control_add_axis_button(&controller->map, "FpsMovement", WR_INPUT_KEYBOARD, WR_KEY_W, (vec3){0, 0, -1},
                                   true);
    walrus_control_add_axis_button(&controller->map, "FpsMovement", WR_INPUT_KEYBOARD, WR_KEY_S, (vec3){0, 0, 1}, true);
    walrus_control_add_axis_button(&controller->map, "FpsMovement", WR_INPUT_KEYBOARD, WR_KEY_A, (vec3){-1, 0, 0},
                                   true);
    walrus_control_add_axis_button(&controller->map, "FpsMovement", WR_INPUT_KEYBOARD, WR_KEY_D, (vec3){1, 0, 0}, true);
    walrus_control_add_axis_axis(&controller->map, "FpsRotation", WR_INPUT_MOUSE, WR_MOUSE_AXIS_CURSOR,
                                 (vec3){-1, -1, 0});

    glm_vec3_zero(controller->translation);
    glm_vec3_zero(controller->smooth_translation);
    glm_vec2_zero(controller->rotation);
    glm_vec2_zero(controller->smooth_rotation);

    controller->speed = speed;
    glm_vec2_copy(rotation_speed, controller->rotate_speed);
    controller->smoothness = smoothness;

    glm_mat3_identity(controller->ground_transform);
}

void walrus_fps_controller_shutdown(Walrus_FpsController *controller)
{
    walrus_control_map_shutdown(&controller->map);
}

static void axis_angles(vec3 axis, f32 angle, versor quat)
{
    f32 ha = angle * 0.5f;
    f32 sa = sin(ha);
    glm_quat_init(quat, axis[0] * sa, axis[1] * sa, axis[2] * sa, cos(ha));
}

static void to_euler(versor quat, vec3 euler)
{
    f32 const sinr = 2.0 * (quat[3] * quat[0] + quat[1] * quat[2]);
    f32 const cosr = 1 - 2.0 * (quat[0] * quat[0] + quat[1] * quat[1]);
    euler[0]       = atan2(sinr, cosr);

    f32 const sinp = sqrt(1 + 2.0 * (quat[3] * quat[1] - quat[0] * quat[2]));
    f32 const cosp = sqrt(1 - 2.0 * (quat[3] * quat[1] - quat[0] * quat[2]));
    euler[1]       = 2 * atan2(sinp, cosp) - M_PI / 2.0;

    f32 const siny = 2.0 * (quat[3] * quat[2] + quat[0] * quat[1]);
    f32 const cosy = 1 - 2.0 * (quat[1] * quat[1] + quat[2] * quat[2]);
    euler[2]       = atan2(siny, cosy);
}

void walrus_fps_controller_tick(Walrus_ControllerEvent *event)
{
    Walrus_FpsController *fc        = event->userdata;
    f32 const             dt        = event->delta_time;
    Walrus_Transform     *transform = event->transform;

    walrus_control_map_tick(&fc->map, fc);

    glm_vec3_scale(fc->translation, dt, fc->translation);
    glm_vec3_lerp(fc->smooth_translation, fc->translation, walrus_clamp(dt * fc->smoothness, 0.f, 1.0f),
                  fc->smooth_translation);
    glm_vec3_zero(fc->translation);

    glm_vec2_scale(fc->rotation, dt, fc->rotation);
    glm_vec2_lerp(fc->smooth_rotation, fc->rotation, walrus_clamp(dt * fc->smoothness, 0.f, 1.0f), fc->smooth_rotation);
    glm_vec2_zero(fc->rotation);

    if (glm_vec2_norm2(fc->smooth_rotation) > 0) {
        versor q;
        vec3   right;
        walrus_transform_right(transform, right);
        vec3 euler;
        to_euler(transform->rot, euler);
        if (walrus_abs(euler[0] + glm_rad(fc->smooth_rotation[1])) < glm_rad(89.0)) {
            axis_angles(right, glm_rad(fc->smooth_rotation[1]), q);
            glm_quat_mul(q, transform->rot, transform->rot);
        }
        axis_angles((vec3){0, 1, 0}, glm_rad(fc->smooth_rotation[0]), q);
        glm_quat_mul(q, transform->rot, transform->rot);

        walrus_transform_right(transform, fc->ground_transform[0]);
        walrus_transform_front(transform, fc->ground_transform[2]);
        fc->ground_transform[2][1] = 0;
        glm_vec3_normalize(fc->ground_transform[2]);
    }

    if (glm_vec3_norm2(fc->smooth_translation) > 0) {
        vec3 local;
        glm_mat3_mulv(fc->ground_transform, fc->smooth_translation, local);
        walrus_transform_translate(transform, local);
    }
}
