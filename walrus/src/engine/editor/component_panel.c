#include <engine/editor/component_panel.h>
#include <engine/editor.h>
#include <engine/component.h>
#include <core/math.h>

static bool igVec3(char const *label, f32 *values, f32 reset_value, f32 column_width)
{
    bool     ret      = false;
    ImGuiIO *io       = igGetIO();
    ImFont  *boldfont = io->Fonts->Fonts.Data[0];

    igPushID_Str(label);

    igColumns(2, NULL, NULL);
    igSetColumnWidth(0, column_width);
    igTextUnformatted(label, NULL);
    igNextColumn();

    igPushMultiItemsWidths(3, igCalcItemWidth());
    igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){0, 0});

    float  line_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 button_size = {line_height + 3.0f, line_height};

    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.8f, 0.1f, 0.15f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.9f, 0.2f, 0.2f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.8f, 0.1f, 0.15f, 1.0f});
    igPushFont(boldfont);
    if (igButton("X", button_size)) {
        ret |= true;
        values[0] = reset_value;
    }
    igPopFont();
    igPopStyleColor(3);

    igSameLine(0.f, -1.0f);
    ret |= igDragFloat("##X", &values[0], 0.1f, 0.0f, 0.0f, "%.2f", 0);
    igPopItemWidth();
    igSameLine(0.f, -1.0f);

    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.2f, 0.7f, 0.2f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.3f, 0.8f, 0.3f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.2f, 0.7f, 0.2f, 1.0f});
    igPushFont(boldfont);
    if (igButton("Y", button_size)) {
        ret |= true;
        values[1] = reset_value;
    }
    igPopFont();
    igPopStyleColor(3);

    igSameLine(0.f, -1.0f);
    ret |= igDragFloat("##Y", &values[1], 0.1f, 0.0f, 0.0f, "%.2f", 0);
    igPopItemWidth();
    igSameLine(0.f, -1.0f);

    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.1f, 0.25f, 0.8f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.2f, 0.35f, 0.9f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.1f, 0.25f, 0.8f, 1.0f});
    igPushFont(boldfont);
    if (igButton("Z", button_size)) {
        ret |= true;
        values[2] = reset_value;
    }
    igPopFont();
    igPopStyleColor(3);

    igSameLine(0.f, -1.0f);
    ret |= igDragFloat("##Z", &values[2], 0.1f, 0.0f, 0.0f, "%.2f", 0);
    igPopItemWidth();

    igPopStyleVar(1);

    igColumns(1, NULL, true);

    igPopID();

    return ret;
}

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
