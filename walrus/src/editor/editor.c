#include <editor/editor.h>
#include <engine/engine.h>
#include <engine/systems/transform_system.h>

ECS_COMPONENT_DECLARE(Walrus_EditorRenderer);
ECS_COMPONENT_DECLARE(Walrus_TransformGuizmo);

ECS_SYSTEM_DECLARE(editor_ui);
ECS_SYSTEM_DECLARE(transform_guizmo_ui);

static void editor_ui(ecs_iter_t *it)
{
    Walrus_EditorRenderer *renderers = ecs_field(it, Walrus_EditorRenderer, 1);
    Walrus_Camera         *cameras   = ecs_field(it, Walrus_Camera, 2);

    u32 width, height;
    walrus_rhi_get_resolution(&width, &height);
    walrus_imgui_new_frame(width, height, 255);
    for (i32 i = 0; i < it->count; ++i) {
        ImGuizmo_SetRect(0, 0, renderers[i].width, renderers[i].height);
        ImGuizmo_SetOrthographic(false);
        ecs_run(it->world, ecs_id(transform_guizmo_ui), 0, &cameras[i]);
    }

    walrus_imgui_end_frame();
}

static void transform_guizmo_ui(ecs_iter_t *it)
{
    Walrus_Transform       *transforms = ecs_field(it, Walrus_Transform, 1);
    Walrus_TransformGuizmo *uis        = ecs_field(it, Walrus_TransformGuizmo, 2);

    Walrus_Camera *camera = it->param;

    for (i32 i = 0; i < it->count; ++i) {
        mat4 world;
        walrus_transform_compose(&transforms[i], world);
        if (ImGuizmo_Manipulate(camera->view[0], camera->projection[0], uis[i].op, uis[i].mode, world[0], NULL, NULL,
                                NULL, NULL)) {
            walrus_transform_decompose(&transforms[i], world);
        }
    }
}

void walrus_editor_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_EditorRenderer);
    ECS_COMPONENT_DEFINE(ecs, Walrus_TransformGuizmo);

    ECS_SYSTEM_DEFINE(ecs, editor_ui, 0, Walrus_EditorRenderer, Walrus_Camera);
    ECS_SYSTEM_DEFINE(ecs, transform_guizmo_ui, 0, Walrus_Transform, Walrus_TransformUI);
}

void walrus_editor_system_render(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ecs_run(ecs, ecs_id(editor_ui), 0, NULL);
}
