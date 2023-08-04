#include <engine/editor/component_panel.h>
#include <engine/editor.h>
#include <engine/component.h>
#include <core/math.h>

static void component_ui(ecs_world_t *ecs, ecs_entity_t e)
{
    if (!ecs_has(ecs, e, Walrus_EntityObserver)) {
        return;
    }

    ecs_entity_t target = ecs_get(ecs, e, Walrus_EntityObserver)->entity;
    if (ecs_has(ecs, target, Walrus_Transform)) {
        Walrus_Transform *transform = ecs_get_mut(ecs, target, Walrus_Transform);
        if (igVec3("Translation", transform->trans, 0, 100.0f)) {
            ecs_modified(ecs, target, Walrus_Transform);
        }
        vec3 eulers;
        glm_to_euler(transform->rot, eulers);
        if (igVec3("Rotation", eulers, 0.f, 100.f)) {
            glm_to_quat(eulers, transform->rot);
            ecs_modified(ecs, target, Walrus_Transform);
        }
        if (igVec3("Scale", transform->scale, 1.0f, 100.0f)) {
            ecs_modified(ecs, target, Walrus_Transform);
        }
    }
    if (ecs_has(ecs, target, Walrus_LocalTransform)) {
        Walrus_LocalTransform *transform = ecs_get_mut(ecs, target, Walrus_LocalTransform);
        if (igVec3("LTranslation", transform->trans, 0, 100.0f)) {
            ecs_modified(ecs, target, Walrus_LocalTransform);
        }
        vec3 eulers;
        glm_to_euler(transform->rot, eulers);
        if (igVec3("LRotation", eulers, 0.f, 100.f)) {
            glm_to_quat(eulers, transform->rot);
            ecs_modified(ecs, target, Walrus_LocalTransform);
        }
        if (igVec3("LScale", transform->scale, 1.0f, 100.0f)) {
            ecs_modified(ecs, target, Walrus_LocalTransform);
        }
    }

    if (ecs_has(ecs, e, Walrus_TransformGuizmo)) {
        Walrus_TransformGuizmo *guizmo = ecs_get_mut(ecs, e, Walrus_TransformGuizmo);
        if (igRadioButton_Bool("Translate", guizmo->op == TRANSLATE)) {
            guizmo->op = TRANSLATE;
            ecs_modified(ecs, e, Walrus_TransformGuizmo);
        }
        if (igRadioButton_Bool("Rotate", guizmo->op == ROTATE)) {
            guizmo->op = ROTATE;
            ecs_modified(ecs, e, Walrus_TransformGuizmo);
        }
        if (igRadioButton_Bool("Scale", guizmo->op == SCALE)) {
            guizmo->op = SCALE;
            ecs_modified(ecs, e, Walrus_TransformGuizmo);
        }
    }
}

ecs_entity_t walrus_add_component_panel(ecs_world_t *ecs, ecs_entity_t window, u32 priority)
{
    ecs_entity_t widget = ecs_new_w_pair(ecs, EcsChildOf, window);
    ecs_set(ecs, widget, Walrus_EditorWidget, {.priority = priority, .func = component_ui});
    ecs_set(ecs, widget, Walrus_TransformGuizmo, {.op = TRANSLATE, .mode = WORLD});
    return widget;
}
