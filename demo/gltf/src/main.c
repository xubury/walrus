#include <engine/engine.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/assert.h>
#include <engine/model.h>
#include <engine/imgui.h>
#include <engine/animator.h>
#include <engine/fps_controller.h>
#include <engine/systems/transform_system.h>
#include <engine/systems/animator_system.h>
#include <engine/systems/model_system.h>
#include <engine/systems/render_system.h>
#include <engine/systems/camera_system.h>
#include <engine/systems/controller_system.h>
#include <editor/editor.h>

static void controller_shutdown(void *controller)
{
    walrus_fps_controller_shutdown(controller);
    walrus_free(controller);
}

static void hello_world_ui(void)
{
    igText("This is some useful text");
    igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
    static i32 lod = 0;
    igInputInt("LOD", &lod, 1, 1, 0);
}

static void transform_ui(void)
{
}

Walrus_AppError on_init(Walrus_App *app)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    walrus_editor_system_init();

    Walrus_FpsController *fps_controller = walrus_new(Walrus_FpsController, 1);
    walrus_fps_controller_init(fps_controller, 10.0, (vec2){3.0, 3.0}, 20.0);

    ecs_entity_t camera = ecs_new_id(ecs);
    ecs_set(ecs, camera, Walrus_Transform, {.rot = {0, 0, 0, 1}, .trans = {0, 2, 5}, .scale = {1, 1, 1}});
    ecs_set(ecs, camera, Walrus_Controller,
            {.tick = walrus_fps_controller_tick, .shutdown = controller_shutdown, .userdata = fps_controller});
    ecs_set(ecs, camera, Walrus_Camera,
            {.fov = glm_rad(45.0), .aspect = 1440.0 / 900, .near_z = 0.01, .far_z = 1000.0});
    ecs_set(ecs, camera, Walrus_Renderer,
            {.x = 0, .y = 0, .width = 1440, .height = 900, .active = true, .framebuffer = {WR_INVALID_HANDLE}});
    /* ecs_set(ecs, camera, Walrus_EditorWindow, {.name = "213"}); */

    walrus_model_system_load_from_file("shibahu", "assets/gltf/shibahu/scene.gltf");
    walrus_model_system_load_from_file("cubes", "assets/gltf/EmissiveStrengthTest.gltf");

    ecs_entity_t character =
        walrus_model_instantiate("shibahu", (vec3){-2, 0, 0}, (versor){0, 0, 0, 1}, (vec3){1, 1, 1});
    ecs_set(ecs, character, Walrus_Animator, {0});
    ecs_set(ecs, character, Walrus_TransformGuizmo, {.op = TRANSLATE, .mode = WORLD});
    ecs_entity_t cubes = walrus_model_instantiate("cubes", (vec3){0, 0, 0}, (versor){0, 0, 0, 1}, (vec3){1, 1, 1});
    ecs_set(ecs, cubes, Walrus_TransformGuizmo, {.op = TRANSLATE, .mode = WORLD});

    ecs_entity_t window = ecs_set(ecs, 0, Walrus_EditorWindow, {.name = "hello world"});
    ecs_set(ecs, ecs_new_w_pair(ecs, EcsChildOf, window), Walrus_EditorWidget, {.func = hello_world_ui});

    ecs_entity_t widget = ecs_new_w_pair(ecs, EcsChildOf, window);
    ecs_set(ecs, widget, Walrus_EditorWidget, {.func = transform_ui});

    return WR_APP_SUCCESS;
}

void on_render(Walrus_App *app)
{
    walrus_unused(app);

    walrus_editor_system_render();
}

void on_event(Walrus_App *app, Walrus_Event *e)
{
    walrus_unused(app);

    if (e->type == WR_EVENT_TYPE_BUTTON) {
        if (e->button.device == WR_INPUT_KEYBOARD && e->button.button == WR_KEY_ESCAPE) {
            walrus_engine_exit();
        }
    }
}

int main(void)
{
    Walrus_App app = {
        .init = on_init, .tick = NULL, .render = on_render, .event = on_event, .shutdown = NULL, .userdata = NULL};

    walrus_engine_init_run("gltf", 1440, 900, &app);

    return 0;
}
