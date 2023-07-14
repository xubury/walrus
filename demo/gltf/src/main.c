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

static void hello_world_ui(ecs_world_t *ecs, ecs_entity_t e)
{
    walrus_unused(ecs);
    walrus_unused(e);

    igText("This is some useful text");
    igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
    static i32 lod = 0;
    igInputInt("LOD", &lod, 1, 1, 0);
}

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

    float  lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.8f, 0.1f, 0.15f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.9f, 0.2f, 0.2f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.8f, 0.1f, 0.15f, 1.0f});
    igPushFont(boldfont);
    if (igButton("X", buttonSize)) {
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
    if (igButton("Y", buttonSize)) {
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
    if (igButton("Z", buttonSize)) {
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
static void to_quat(vec3 euler, versor q)
{
    f32 cr = cos(euler[0] * 0.5);
    f32 sr = sin(euler[0] * 0.5);
    f32 cp = cos(euler[1] * 0.5);
    f32 sp = sin(euler[1] * 0.5);
    f32 cy = cos(euler[2] * 0.5);
    f32 sy = sin(euler[2] * 0.5);

    q[0] = cr * cp * cy + sr * sp * sy;
    q[1] = sr * cp * cy - cr * sp * sy;
    q[2] = cr * sp * cy + sr * cp * sy;
    q[3] = cr * cp * sy - sr * sp * cy;
}

static void to_euler(versor quat, vec3 euler)
{
    f32 const sinr = 2.0 * (quat[3] * quat[0] + quat[1] * quat[2]);
    f32 const cosr = 1 - 2.0 * (quat[0] * quat[0] + quat[1] * quat[1]);
    euler[0]       = atan2(sinr, cosr);

    f32 const sinp = sqrt(1 + 2.0 * (quat[3] * quat[1] - quat[0] * quat[2]));
    f32 const cosp = sqrt(1 - 2.0 * (quat[3] * quat[1] - quat[0] * quat[2]));
    euler[1]       = 2 * atan2(sinp, cosp) - M_PI / 2.0;

    f32 const siny = 2.0 * (quat[3] * quat[2] + quat[0] * quat[1]);
    f32 const cosy = 1 - 2.0 * (quat[1] * quat[1] + quat[2] * quat[2]);
    euler[2]       = atan2(siny, cosy);
}

static void component_ui(ecs_world_t *ecs, ecs_entity_t e)
{
    walrus_assert(ecs_has(ecs, e, Walrus_EntityObserver));

    ecs_entity_t target = ecs_get(ecs, e, Walrus_EntityObserver)->entity;
    if (ecs_has(ecs, target, Walrus_Transform)) {
        Walrus_Transform *transform = ecs_get_mut(ecs, target, Walrus_Transform);
        igVec3("translate", transform->trans, 0, 100.0f);
        vec3 eulers;
        to_euler(transform->rot, eulers);
        if (igVec3("rotation", eulers, 0.f, 100.f)) {
            to_quat(eulers, transform->rot);
        }
        igVec3("scale", transform->scale, 1.0f, 100.0f);
    }

    if (ecs_has(ecs, target, Walrus_TransformGuizmo)) {
        Walrus_TransformGuizmo *guizmo = ecs_get_mut(ecs, target, Walrus_TransformGuizmo);
        if (igRadioButton_Bool("Translate", guizmo->op == TRANSLATE)) {
            guizmo->op = TRANSLATE;
        }
        if (igRadioButton_Bool("Rotate", guizmo->op == ROTATE)) {
            guizmo->op = ROTATE;
        }
        if (igRadioButton_Bool("Scale", guizmo->op == SCALE)) {
            guizmo->op = SCALE;
        }
    }
}

Walrus_AppError on_init(Walrus_App *app)
{
    walrus_unused(app);

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

    ecs_entity_t observer = ecs_set(ecs, 0, Walrus_EntityObserver, {.entity = cubes});
    ecs_entity_t widget   = ecs_new_w_pair(ecs, EcsChildOf, window);
    ecs_set(ecs, widget, Walrus_EditorWidget, {.func = component_ui});
    ecs_add_pair(ecs, widget, EcsIsA, observer);

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
