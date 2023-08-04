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

static bool has_child(ecs_world_t *ecs, ecs_entity_t e)
{
    ecs_iter_t it = ecs_children(ecs, e);
    ecs_children_next(&it);
    return it.count > 0;
}

static bool child_hierarchy_ui(ecs_world_t *ecs, Walrus_EntityObserver *ob, ecs_entity_t e)
{
    bool          mod = false;
    ecs_filter_t *f   = ecs_filter(ecs, {.terms = {
                                           {.id = ecs_id(Walrus_Transform)},
                                           {.id = ecs_pair(EcsChildOf, e)},
                                       }});

    ecs_iter_t         it = ecs_filter_iter(ecs, f);
    ImGuiTreeNodeFlags base_flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
    while (ecs_filter_next(&it)) {
        for (i32 i = 0; i < it.count; ++i) {
            ImGuiTreeNodeFlags flags = has_child(it.world, it.entities[i]) ? base_flags : leaf_flags;
            flags |=
                (ob && ob->entity == it.entities[i]) ? ImGuiTreeNodeFlags_Selected : 0 | ImGuiTreeNodeFlags_OpenOnArrow;
            char const *name = ecs_get_name(it.world, it.entities[i]);
            bool opened = igTreeNodeEx_Ptr(walrus_val_to_ptr(it.entities[i]), flags, "%s", name ? name : "unnamed");
            if (ob && igIsItemClicked(0)) {
                ob->entity = it.entities[i];
                mod        = true;
            }
            if (opened) {
                child_hierarchy_ui(ecs, ob, it.entities[i]);
                igTreePop();
            }
        }
    }
    ecs_filter_fini(f);
    return mod;
}

static void scene_hierarchy_ui(ecs_world_t *ecs, ecs_entity_t e)
{
    ecs_entity_t           base = ecs_get_target_for_id(ecs, e, EcsIsA, ecs_id(Walrus_EntityObserver));
    Walrus_EntityObserver *ob   = NULL;
    if (base) {
        ob = ecs_get_mut(ecs, base, Walrus_EntityObserver);
    }

    ecs_filter_t *f =
        ecs_filter(ecs, {.terms = {
                             {.id = ecs_id(Walrus_Transform)},
                             {.id = ecs_id(Walrus_Transform), .src = {.flags = EcsCascade | EcsParent}, .oper = EcsNot},
                         }});

    ecs_iter_t it = ecs_filter_iter(ecs, f);

    ImGuiTreeNodeFlags base_flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
    while (ecs_filter_next(&it)) {
        for (i32 i = 0; i < it.count; ++i) {
            ImGuiTreeNodeFlags flags = has_child(it.world, it.entities[i]) ? base_flags : leaf_flags;
            flags |=
                (ob && ob->entity == it.entities[i]) ? ImGuiTreeNodeFlags_Selected : 0 | ImGuiTreeNodeFlags_OpenOnArrow;
            char const *name = ecs_get_name(it.world, it.entities[i]);
            bool opened = igTreeNodeEx_Ptr(walrus_val_to_ptr(it.entities[i]), flags, "%s", name ? name : "unnamed");
            if (ob && igIsItemClicked(0)) {
                ob->entity = it.entities[i];
                ecs_modified(ecs, base, Walrus_EntityObserver);
            }
            if (opened) {
                if (child_hierarchy_ui(ecs, ob, it.entities[i])) {
                    ecs_modified(ecs, base, Walrus_EntityObserver);
                }
                igTreePop();
            }
        }
    }
    ecs_filter_fini(f);
}

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
    ecs_set_name(ecs, camera, "camera");

    walrus_model_system_load_from_file(model, "shibahu", "assets/gltf/shibahu/scene.gltf");
    walrus_model_system_load_from_file(model, "cubes", "assets/gltf/EmissiveStrengthTest.gltf");

    ecs_entity_t character =
        walrus_model_instantiate(model, "shibahu", (vec3){-2, 0, 0}, (versor){0, 0, 0, 1}, (vec3){1, 1, 1});
    ecs_set(ecs, character, Walrus_Animator, {0});
    ecs_set_name(ecs, character, "character");

    ecs_entity_t cubes =
        walrus_model_instantiate(model, "cubes", (vec3){0, 0, 0}, (versor){0, 0, 0, 1}, (vec3){1, 1, 1});
    ecs_set_name(ecs, cubes, "cubes");

    ecs_entity_t observer = ecs_set(ecs, 0, Walrus_EntityObserver, {.entity = cubes});

    ecs_entity_t window = ecs_set(ecs, 0, Walrus_EditorWindow, {.name = "hello world"});

    ecs_set(ecs, ecs_new_w_pair(ecs, EcsChildOf, window), Walrus_EditorWidget, {.priority = 0, .func = hello_world_ui});

    ecs_entity_t hierarchy = ecs_new_w_pair(ecs, EcsChildOf, window);
    ecs_add_pair(ecs, hierarchy, EcsIsA, observer);
    ecs_set(ecs, hierarchy, Walrus_EditorWidget, {.priority = 2, .func = scene_hierarchy_ui});

    ecs_entity_t widget = walrus_add_component_panel(ecs, window, 1);
    ecs_add_pair(ecs, widget, EcsIsA, observer);

    walrus_assert(ecs_has(ecs, widget, Walrus_EntityObserver));

    // TODO: serialize/deserialize test
    {
        // Walrus_Transform const *t = ecs_get(ecs, character, Walrus_Transform);
        // walrus_trace("%f, %f, %f", t->trans[0], t->trans[1], t->trans[2]);
        // char const *expr = ecs_ptr_to_expr(ecs, ecs_id(Walrus_Transform), ecs_get(ecs, character, Walrus_Transform));
        // walrus_trace("serialize: %s", expr);
        // Walrus_Transform transform = {0};
        // ecs_parse_expr(ecs, expr, &ecs_value(Walrus_Transform, &transform), NULL);
        // walrus_trace("%f, %f, %f", transform.trans[0], transform.trans[1], transform.trans[2]);
        // walrus_trace("%f, %f, %f, %f", transform.rot[0], transform.rot[1], transform.rot[2], transform.rot[3]);
        // walrus_trace("%f, %f, %f", transform.scale[0], transform.scale[1], transform.scale[2]);
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

POLY_DEFINE_DERIVED(Walrus_App, void, my_app_create, POLY_IMPL(on_app_init, on_init), POLY_IMPL(on_app_event, on_event))

int main(void)
{
    Walrus_App app = my_app_create(NULL, NULL);
    walrus_engine_init_run("gltf", 1440, 900, &app);

    // Just in case
    poly_free(&app);

    return 0;
}
