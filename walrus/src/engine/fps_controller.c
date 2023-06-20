#include <engine/fps_controller.h>
#include <engine/engine.h>
#include <core/math.h>

static void fps_front_back(f32 scale, void *userdata)
{
    Walrus_FpsController *controller = userdata;

    glm_vec3_add(controller->translation, (vec3){0, 0, scale}, controller->translation);
}

static void fps_left_right(f32 scale, void *userdata)
{
    Walrus_FpsController *controller = userdata;

    glm_vec3_add(controller->translation, (vec3){scale, 0, 0}, controller->translation);
}

void walrus_fps_controller_init(Walrus_FpsController *controller, f32 smoothness)
{
    Walrus_ControlMap *control = walrus_engine_control();
    walrus_control_bind_axis(control, "FpsControllerFrontBack", fps_front_back, controller);
    walrus_control_bind_axis(control, "FpsControllerLeftRight", fps_left_right, controller);
    walrus_control_add_axis_button(control, "FpsControllerFrontBack", WR_INPUT_KEYBOARD, WR_KEY_W, -1, true);
    walrus_control_add_axis_button(control, "FpsControllerFrontBack", WR_INPUT_KEYBOARD, WR_KEY_S, 1, true);
    walrus_control_add_axis_button(control, "FpsControllerLeftRight", WR_INPUT_KEYBOARD, WR_KEY_A, -1, true);
    walrus_control_add_axis_button(control, "FpsControllerLeftRight", WR_INPUT_KEYBOARD, WR_KEY_D, 1, true);

    glm_vec3_zero(controller->translation);
    glm_vec3_zero(controller->smooth_translation);
    controller->smoothness = smoothness;
}

void walrus_fps_controller_shutdown(Walrus_FpsController *controller)
{
    walrus_unused(controller);

    Walrus_ControlMap *control = walrus_engine_control();
    walrus_control_clear(control, "FpsControllerFrontBack");
    walrus_control_clear(control, "FpsControllerLeftRight");
}

void walrus_fps_controller_tick(Walrus_FpsController *controller, Walrus_Transform *transform, f32 dt)
{
    glm_vec3_lerp(controller->smooth_translation, controller->translation,
                  walrus_clamp(dt * controller->smoothness, 0.f, 1.0f), controller->smooth_translation);
    glm_vec3_zero(controller->translation);

    walrus_transfrom_translate(transform, controller->smooth_translation);
}
