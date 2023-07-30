#include <engine/fps_controller.h>
#include <engine/engine.h>
#include <core/math.h>
#include <core/memory.h>

static void axis_angles(vec3 axis, f32 angle, versor quat)
{
    f32 ha = angle * 0.5f;
    f32 sa = sin(ha);
    glm_quat_init(quat, axis[0] * sa, axis[1] * sa, axis[2] * sa, cos(ha));
}

static void fps_controller_init(Walrus_Controller *controller)
{
    Walrus_FpsController *fc = poly_cast(controller, Walrus_FpsController);
    glm_vec3_zero(fc->smooth_translation);
    glm_vec2_zero(fc->smooth_rotation);
    glm_mat3_identity(fc->ground_transform);

    walrus_input_add_axis_button(&controller->map, "FpsMovement", WR_INPUT_KEYBOARD, WR_KEY_W, (vec3){0, 0, -1}, true);
    walrus_input_add_axis_button(&controller->map, "FpsMovement", WR_INPUT_KEYBOARD, WR_KEY_S, (vec3){0, 0, 1}, true);
    walrus_input_add_axis_button(&controller->map, "FpsMovement", WR_INPUT_KEYBOARD, WR_KEY_A, (vec3){-1, 0, 0}, true);
    walrus_input_add_axis_button(&controller->map, "FpsMovement", WR_INPUT_KEYBOARD, WR_KEY_D, (vec3){1, 0, 0}, true);
    walrus_input_add_axis_axis(&controller->map, "FpsRotation", WR_INPUT_MOUSE, WR_MOUSE_AXIS_CURSOR,
                               (vec3){-1, -1, 0});
}

static void fps_controller_tick(Walrus_Controller *controller, Walrus_ControllerEvent *event)
{
    Walrus_FpsController *fc        = poly_cast(controller, Walrus_FpsController);
    f32 const             dt        = event->delta_time;
    Walrus_Transform     *transform = event->transform;
    Walrus_InputMap      *map       = &controller->map;

    vec3 translation = GLM_VEC3_ZERO_INIT;
    if (walrus_input_get_axis(map, "FpsMovement", translation)) {
        glm_vec3_mul(translation, (vec3){fc->speed, fc->speed, fc->speed}, translation);
        glm_vec3_scale(translation, dt, translation);
    }
    vec3 rotation = GLM_VEC3_ZERO_INIT;
    if (walrus_input_get_axis(map, "FpsRotation", rotation)) {
        glm_vec2_mul(rotation, fc->rotate_speed, rotation);
        glm_vec2_clamp(rotation, -180, 180);
        glm_vec2_scale(rotation, dt, rotation);
    }

    glm_vec3_lerp(fc->smooth_translation, translation, walrus_clamp(dt * fc->smoothness, 0.f, 1.0f),
                  fc->smooth_translation);

    glm_vec2_lerp(fc->smooth_rotation, rotation, walrus_clamp(dt * fc->smoothness, 0.f, 1.0f), fc->smooth_rotation);

    if (glm_vec2_norm2(fc->smooth_rotation) > 0) {
        versor q;
        vec3   right;
        walrus_transform_right(transform, right);
        vec3 euler;
        glm_to_euler(transform->rot, euler);
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

POLY_DEFINE_DERIVED(Walrus_Controller, Walrus_FpsController, walrus_fps_controller,
                    POLY_IMPL(init, fps_controller_init), POLY_IMPL(tick, fps_controller_tick))
