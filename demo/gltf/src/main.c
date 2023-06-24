#include <engine/engine.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/assert.h>
#include <engine/model.h>
#include <engine/imgui.h>
#include <rhi/rhi.h>
#include <engine/batch_renderer.h>
#include <engine/deferred_renderer.h>
#include <engine/shader_library.h>
#include <engine/thread_pool.h>
#include <engine/animator.h>
#include <engine/camera.h>
#include <engine/fps_controller.h>
#include <core/sys.h>
#include <core/array.h>

#include <cglm/cglm.h>
#include <string.h>

typedef struct {
    ecs_entity_t camera;
    ecs_entity_t character;

    ecs_entity_t model;
} AppData;

Walrus_AppError on_init(Walrus_App *app)
{
    AppData *data = app->userdata;

    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT(ecs, Walrus_FpsController);
    ECS_COMPONENT(ecs, Walrus_Transform);
    ECS_COMPONENT(ecs, Walrus_Camera);
    ECS_COMPONENT(ecs, Walrus_DeferredRenderer);

    data->camera = ecs_new_id(ecs);
    ecs_set(ecs, data->camera, Walrus_Transform, {.rot = {0, 0, 0, 1}, .trans = {0, 2, 5}, .scale = {1, 1, 1}});
    ecs_set(ecs, data->camera, Walrus_FpsController, {.speed = 10.0, .rotate_speed = {3.0, 3.0}, .smoothness = 20.0});
    ecs_set(ecs, data->camera, Walrus_Camera,
            {.fov = glm_rad(45.0), .aspect = 1440.0 / 900, .near_z = 0.01, .far_z = 1000.0});
    ecs_set(ecs, data->camera, Walrus_DeferredRenderer,
            {.x = 0, .y = 0, .width = 1440, .height = 900, .framebuffer = {WR_INVALID_HANDLE}});

    ECS_COMPONENT(ecs, Walrus_Model);
    data->model            = ecs_set(ecs, 0, Walrus_Model, {0});
    Walrus_Model *model    = ecs_get_mut(ecs, data->model, Walrus_Model);
    u64           ts       = walrus_sysclock(WR_SYS_CLOCK_UNIT_MILLSEC);
    char const   *filename = "assets/gltf/shibahu/scene.gltf";
    if (walrus_model_load_from_file(model, filename) != WR_MODEL_SUCCESS) {
        walrus_error("error loading model from: %s !", filename);
    }
    walrus_trace("model load time: %llu ms", walrus_sysclock(WR_SYS_CLOCK_UNIT_MILLSEC) - ts);

    ECS_COMPONENT(ecs, Walrus_Animator);
    data->character = ecs_new_w_pair(ecs, EcsIsA, data->model);
    ecs_set(ecs, data->character, Walrus_Transform, {.rot = {0, 0, 0, 1}, .trans = {0, 0, 0}, .scale = {1, 1, 1}});
    ecs_set(ecs, data->character, Walrus_Animator, {0});

    ecs_entity_t e = ecs_new_w_pair(ecs, EcsIsA, data->model);
    ecs_set(ecs, e, Walrus_Transform, {.rot = {0, 0, 0, 1}, .trans = {2, 0, 0}, .scale = {1, 1, 1}});

    return WR_APP_SUCCESS;
}

void on_render(Walrus_App *app)
{
    AppData     *data = app->userdata;
    ecs_world_t *ecs  = walrus_engine_vars()->ecs;
    ECS_COMPONENT(ecs, Walrus_Camera);
    ECS_COMPONENT(ecs, Walrus_Transform);
    Walrus_Camera const *camera    = ecs_get(ecs, data->camera, Walrus_Camera);
    Walrus_Transform    *transform = ecs_get_mut(ecs, data->character, Walrus_Transform);

    u32 width, height;
    walrus_rhi_get_resolution(&width, &height);
    walrus_imgui_new_frame(width, height, 255);

    ImGuizmo_SetRect(0, 0, width, height);
    ImGuizmo_SetOrthographic(false);
    mat4 world;
    walrus_transform_compose(transform, world);
    if (ImGuizmo_Manipulate(camera->view[0], camera->projection[0], TRANSLATE, WORLD, world[0], NULL, NULL, NULL,
                            NULL)) {
        walrus_transform_decompose(transform, world);
    }

    igBegin("Hello, world!", NULL, 0);
    igText("This is some useful text");
    igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
    static i32 lod = 0;
    igInputInt("LOD", &lod, 1, 1, 0);
    igEnd();
    walrus_imgui_end_frame();
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
    Walrus_App app = {.init     = on_init,
                      .tick     = NULL,
                      .render   = on_render,
                      .event    = on_event,
                      .shutdown = NULL,
                      .userdata = walrus_malloc(sizeof(AppData))};

    walrus_engine_init_run("gltf", 1440, 900, &app);

    return 0;
}
