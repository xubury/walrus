#include <engine/engine.h>
#include <core/cpoly.h>
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

static void hello_world_ui(ecs_world_t *ecs, ecs_entity_t e)
{
    walrus_unused(ecs);
    walrus_unused(e);

    igText("This is some useful text");
    igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
    static i32 lod = 0;
    igInputInt("LOD", &lod, 1, 1, 0);
}

typedef i32 MyApp;

static Walrus_AppError on_init(Walrus_App *app)
{
    walrus_unused(app);

    ecs_world_t   *ecs   = walrus_engine_vars()->ecs;
    Walrus_System *model = walrus_engine_vars()->model;
    walrus_assert(ecs == model->ecs);

    ecs_entity_t camera = ecs_new_id(ecs);
    ecs_set(ecs, camera, Walrus_Transform, {.rot = {0, 0, 0, 1}, .trans = {0, 2, 5}, .scale = {1, 1, 1}});

    Walrus_FpsController *fps = walrus_malloc(sizeof(Walrus_FpsController));
    *fps                      = (Walrus_FpsController){.speed = 10, .rotate_speed = {3, 3}, .smoothness = 20};
    Walrus_Controller c       = walrus_fps_controller(fps, walrus_free);

    ecs_set_ptr(ecs, camera, Walrus_Controller, &c);

    ecs_set(ecs, camera, Walrus_Camera,
            {.fov = glm_rad(45.0), .aspect = 1440.0 / 900, .near_z = 0.01, .far_z = 1000.0});
    ecs_set(ecs, camera, Walrus_Renderer,
            {.x = 0, .y = 0, .width = 1440, .height = 900, .active = true, .framebuffer = {WR_INVALID_HANDLE}});

    walrus_model_system_load_from_file(model, "shibahu", "assets/gltf/shibahu/scene.gltf");
    walrus_model_system_load_from_file(model, "cubes", "assets/gltf/EmissiveStrengthTest.gltf");

    ecs_entity_t character =
        walrus_model_instantiate(model, "shibahu", (vec3){-2, 0, 0}, (versor){0, 0, 0, 1}, (vec3){1, 1, 1});
    ecs_set(ecs, character, Walrus_Animator, {0});
    ecs_set(ecs, character, Walrus_TransformGuizmo, {.op = TRANSLATE, .mode = WORLD});
    ecs_entity_t cubes =
        walrus_model_instantiate(model, "cubes", (vec3){0, 0, 0}, (versor){0, 0, 0, 1}, (vec3){1, 1, 1});
    ecs_set(ecs, cubes, Walrus_TransformGuizmo, {.op = TRANSLATE, .mode = WORLD});

    ecs_entity_t window = ecs_set(ecs, 0, Walrus_EditorWindow, {.name = "hello world"});
    ecs_set(ecs, ecs_new_w_pair(ecs, EcsChildOf, window), Walrus_EditorWidget, {.func = hello_world_ui});

    ecs_entity_t observer = ecs_set(ecs, 0, Walrus_EntityObserver, {.entity = cubes});
    ecs_entity_t widget   = walrus_add_component_panel(ecs, window);
    ecs_add_pair(ecs, widget, EcsIsA, observer);

    // TODO: serialize/deserialize test
    {
        Walrus_Transform const *t = ecs_get(ecs, character, Walrus_Transform);
        walrus_trace("%f, %f, %f", t->trans[0], t->trans[1], t->trans[2]);
        char const *expr = ecs_ptr_to_expr(ecs, ecs_id(Walrus_Transform), ecs_get(ecs, character, Walrus_Transform));
        walrus_trace("serialize: %s", expr);
        Walrus_Transform transform = {0};
        ecs_parse_expr(ecs, expr, &ecs_value(Walrus_Transform, &transform), NULL);
        walrus_trace("%f, %f, %f", transform.trans[0], transform.trans[1], transform.trans[2]);
    }

    return WR_APP_SUCCESS;
}

static void on_event(Walrus_App *app, Walrus_Event *e)
{
    walrus_unused(app);

    if (e->type == WR_EVENT_TYPE_BUTTON) {
        if (e->button.device == WR_INPUT_KEYBOARD && e->button.button == WR_KEY_ESCAPE) {
            walrus_engine_exit();
        }
    }
}

POLY_DEFINE_DERIVED(Walrus_App, MyApp, my_app_create, POLY_IMPL(on_app_init, on_init),
                    POLY_IMPL(on_app_event, on_event))

int main(void)
{
    MyApp my_app;

    Walrus_App app = my_app_create(&my_app, NULL);
    walrus_engine_init_run("gltf", 1440, 900, &app);

    // Just in case
    poly_free(&app);

    return 0;
}
