#include <engine/engine.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/math.h>
#include <core/log.h>
#include <core/assert.h>
#include <engine/imgui.h>
#include <engine/fps_controller.h>
#include <engine/component.h>
#include <engine/editor.h>
#include <engine/editor/component_panel.h>
#include <engine/systems/model_system.h>

static void controller_init(Walrus_Controller *controller)
{
    Walrus_FpsController *fc = walrus_new(Walrus_FpsController, 1);
    walrus_fps_controller_init(fc, &controller->map, 10.0, (vec2){3.0, 3.0}, 20.0);
    controller->userdata = fc;
}

static void controller_shutdown(Walrus_Controller *controller)
{
    walrus_free(controller->userdata);
}

static void hello_world_ui(ecs_world_t *ecs, ecs_entity_t e)
{
    walrus_unused(ecs);
    walrus_unused(e);

    igText("This is some useful text");
    igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
    static i32 lod = 0;
    igInputInt("LOD", &lod, 1, 1, 0);
}

Walrus_AppError on_init(Walrus_App *app)
{
    walrus_unused(app);

    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    ecs_entity_t camera = ecs_new_id(ecs);
    ecs_set(ecs, camera, Walrus_Transform, {.rot = {0, 0, 0, 1}, .trans = {0, 2, 5}, .scale = {1, 1, 1}});
    ecs_set(ecs, camera, Walrus_Controller,
            {.tick = walrus_fps_controller_tick, .init = controller_init, .shutdown = controller_shutdown});
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

    ecs_entity_t observer = ecs_set(ecs, 0, Walrus_EntityObserver, {.entity = cubes});
    ecs_entity_t widget   = walrus_add_component_panel(ecs, window);
    ecs_add_pair(ecs, widget, EcsIsA, observer);

    return WR_APP_SUCCESS;
}

void on_render(Walrus_App *app)
{
    walrus_unused(app);
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
