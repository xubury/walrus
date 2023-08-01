#include <engine/systems/editor_system.h>
#include <engine/systems/transform_system.h>
#include <engine/systems/render_system.h>
#include <engine/renderer.h>
#include <rhi/rhi.h>

ECS_COMPONENT_DECLARE(Walrus_TransformGuizmo);
ECS_COMPONENT_DECLARE(Walrus_EditorWindow);
ECS_COMPONENT_DECLARE(Walrus_EditorWidget);
ECS_COMPONENT_DECLARE(Walrus_EntityObserver);

ECS_SYSTEM_DECLARE(editor_window_ui);
ECS_SYSTEM_DECLARE(viewport_editor_ui);
ECS_SYSTEM_DECLARE(transform_guizmo_ui);

static void viewport_editor_ui(ecs_iter_t *it)
{
    Walrus_Renderer *renderers = ecs_field(it, Walrus_Renderer, 1);
    Walrus_Camera   *cameras   = ecs_field(it, Walrus_Camera, 2);

    for (i32 i = 0; i < it->count; ++i) {
        bool has_window = false;
        if (ecs_has(it->world, it->entities[i], Walrus_EditorWindow)) {
            has_window = true;
        }
        if (has_window) {
            Walrus_EditorWindow const *win = ecs_get(it->world, it->entities[i], Walrus_EditorWindow);
            igBegin(win->name, NULL, 0);
            // TODO: draw the main renderer's content with igTexture
            ImGuizmo_SetDrawlist(NULL);
        }

        ImGuizmo_SetRect(0, 0, renderers[i].width, renderers[i].height);
        ImGuizmo_SetOrthographic(false);
        ecs_run(it->world, ecs_id(transform_guizmo_ui), 0, &cameras[i]);

        if (has_window) {
            igEnd();
        }
    }
}

static void transform_guizmo_ui(ecs_iter_t *it)
{
    Walrus_Transform       *transforms = ecs_field(it, Walrus_Transform, 1);
    Walrus_TransformGuizmo *uis        = ecs_field(it, Walrus_TransformGuizmo, 2);

    Walrus_Camera *camera = it->param;

    for (i32 i = 0; i < it->count; ++i) {
        ImGuizmo_SetID(it->entities[i]);
        mat4 world;
        walrus_transform_compose(&transforms[i], world);
        if (ImGuizmo_Manipulate(camera->view[0], camera->projection[0], uis[i].op, uis[i].mode, world[0], NULL, NULL,
                                NULL, NULL)) {
            walrus_transform_decompose(&transforms[i], world);
        }
    }
}

static void editor_window_ui(ecs_iter_t *it)
{
    Walrus_EditorWindow *windows = ecs_field(it, Walrus_EditorWindow, 1);

    for (i32 i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];

        igBegin(windows[i].name, &windows[i].opened, windows[i].flags);
        {
            ecs_filter_t *f =
                ecs_filter_init(it->world, &(ecs_filter_desc_t){.terms = {{.id = ecs_id(Walrus_EditorWidget)},
                                                                          {.id = ecs_pair(EcsChildOf, entity)}}});
            ecs_iter_t child_it = ecs_filter_iter(it->world, f);
            while (ecs_filter_next(&child_it)) {
                Walrus_EditorWidget *widgets = ecs_field(&child_it, Walrus_EditorWidget, 1);
                for (i32 j = 0; j < child_it.count; ++j) {
                    widgets[j].func(child_it.world, child_it.entities[j]);
                }
            }

            ecs_filter_fini(f);
        }
        igEnd();
    }
}

static void editor_system_init(Walrus_System *sys)
{
    ecs_world_t *ecs = sys->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_TransformGuizmo);
    ECS_COMPONENT_DEFINE(ecs, Walrus_EditorWindow);
    ECS_COMPONENT_DEFINE(ecs, Walrus_EditorWidget);
    ECS_COMPONENT_DEFINE(ecs, Walrus_EntityObserver);

    ECS_SYSTEM_DEFINE(ecs, editor_window_ui, 0, Walrus_EditorWindow);
    ECS_SYSTEM_DEFINE(ecs, viewport_editor_ui, 0, Walrus_Renderer, Walrus_Camera);
    ECS_SYSTEM_DEFINE(ecs, transform_guizmo_ui, 0, Walrus_Transform, Walrus_TransformGuizmo);
}

static void editor_system_render(Walrus_System *sys)
{
    ecs_world_t *ecs = sys->ecs;

    u32 width, height;
    walrus_rhi_get_resolution(&width, &height);
    walrus_imgui_new_frame(width, height, 255);

    ecs_run(ecs, ecs_id(editor_window_ui), 0, NULL);
    ecs_run(ecs, ecs_id(viewport_editor_ui), 0, NULL);

    walrus_imgui_end_frame();
}

POLY_DEFINE_DERIVED(Walrus_System, EditorSystem, editor_system_create, POLY_IMPL(on_system_init, editor_system_init),
                    POLY_IMPL(on_system_render, editor_system_render))
