#include <engine/systems/render_system.h>
#include <engine/engine.h>
#include <engine/animator.h>
#include <engine/model.h>
#include <engine/shader_library.h>
#include <engine/deferred_renderer.h>
#include <engine/camera.h>
#include <core/memory.h>

static void deferred_render_animated_model(ecs_iter_t *it)
{
    Walrus_Transform *transforms = ecs_field(it, Walrus_Transform, 1);
    Walrus_Model     *models     = ecs_field(it, Walrus_Model, 2);
    Walrus_Animator  *animators  = ecs_field(it, Walrus_Animator, 3);

    for (i32 i = 0; i < it->count; ++i) {
        walrus_deferred_renderer_submit(&transforms[i], &models[i], &animators[i]);
    }
}

static void deferred_render_model(ecs_iter_t *it)
{
    Walrus_Transform *transforms = ecs_field(it, Walrus_Transform, 1);
    Walrus_Model     *models     = ecs_field(it, Walrus_Model, 2);
    ECS_COMPONENT(it->world, Walrus_Animator);

    for (i32 i = 0; i < it->count; ++i) {
        if (ecs_has(it->world, it->entities[i], Walrus_Animator)) {
            Walrus_Animator const *animator = ecs_get(it->world, it->entities[i], Walrus_Animator);
            walrus_deferred_renderer_submit(&transforms[i], &models[i], animator);
        }
        else {
            walrus_deferred_renderer_submit(&transforms[i], &models[i], NULL);
        }
    }
}

static void deferred_renderer_run(ecs_iter_t *it)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    Walrus_DeferredRenderer *renderers = ecs_field(it, Walrus_DeferredRenderer, 1);
    Walrus_Camera           *cameras   = ecs_field(it, Walrus_Camera, 2);

    ECS_COMPONENT(ecs, Walrus_Transform);
    ECS_COMPONENT(ecs, Walrus_Model);
    ECS_COMPONENT(ecs, Walrus_Animator);
    ECS_SYSTEM(ecs, deferred_render_animated_model, 0, Walrus_Transform, Walrus_Model, Walrus_Animator);
    ECS_SYSTEM(ecs, deferred_render_model, 0, Walrus_Transform, Walrus_Model);

    for (i32 i = 0; i < it->count; ++i) {
        walrus_deferred_renderer_set_camera(&renderers[i], &cameras[i]);
        ecs_run(ecs, ecs_id(deferred_render_animated_model), 0, NULL);
        ecs_run(ecs, ecs_id(deferred_render_model), 0, NULL);
    }
}

void walrus_render_system_init(void)
{
    walrus_deferred_renderer_init_uniforms();
}

void walrus_render_system_render(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT(ecs, Walrus_DeferredRenderer);
    ECS_COMPONENT(ecs, Walrus_Camera);
    ECS_SYSTEM(ecs, deferred_renderer_run, 0, Walrus_DeferredRenderer, Walrus_Camera);
    ecs_run(ecs, ecs_id(deferred_renderer_run), 0, NULL);
}
