#include <engine/systems/pipelines/deferred_pipeline.h>
#include <engine/systems/render_system.h>
#include <engine/component.h>
#include <engine/engine.h>
#include <core/macro.h>

ECS_SYSTEM_DECLARE(deferred_submit_static_mesh);
ECS_SYSTEM_DECLARE(deferred_submit_skinned_mesh);
ECS_SYSTEM_DECLARE(forward_submit_static_mesh);
ECS_SYSTEM_DECLARE(forward_submit_skinned_mesh);

static void gbuffer_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    ecs_run(ecs, ecs_id(deferred_submit_static_mesh), 0, NULL);
    ecs_run(ecs, ecs_id(deferred_submit_skinned_mesh), 0, NULL);

    walrus_trace("render index: %d name: %s", node->index, node->name);
}

static void deferred_lighting_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    walrus_unused(graph);
    walrus_deferred_renderer_lighting();
    walrus_trace("render index: %d name: %s", node->index, node->name);
}

static void forward_lighting_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    ecs_run(ecs, ecs_id(forward_submit_static_mesh), 0, NULL);
    ecs_run(ecs, ecs_id(forward_submit_skinned_mesh), 0, NULL);

    walrus_trace("render index: %d name: %s", node->index, node->name);
}

static void deferred_submit_static_mesh(ecs_iter_t *it)
{
    Walrus_RenderMesh *meshes     = ecs_field(it, Walrus_RenderMesh, 1);
    Walrus_Material   *materials  = ecs_field(it, Walrus_Material, 2);
    Walrus_Transform  *transforms = ecs_field(it, Walrus_Transform, 3);

    for (i32 i = 0; i < it->count; ++i) {
        if (meshes[i].culled) {
            continue;
        }
        if (materials[i].alpha_mode == WR_ALPHA_MODE_BLEND) {
            continue;
        }

        mat4 world;
        walrus_transform_compose(&transforms[i], world);

        Walrus_TransientBuffer const *weights = NULL;
        if (ecs_has(it->world, it->entities[i], Walrus_WeightResource)) {
            weights = &ecs_get(it->world, it->entities[i], Walrus_WeightResource)->weight_buffer;
        }
        walrus_material_submit(&materials[i]);
        walrus_deferred_renderer_submit_mesh(world, meshes[i].mesh, weights);
    }
}

static void deferred_submit_skinned_mesh(ecs_iter_t *it)
{
    Walrus_RenderMesh   *meshes    = ecs_field(it, Walrus_RenderMesh, 1);
    Walrus_Material     *materials = ecs_field(it, Walrus_Material, 2);
    Walrus_SkinResource *skins     = ecs_field(it, Walrus_SkinResource, 3);

    for (i32 i = 0; i < it->count; ++i) {
        if (meshes[i].culled) {
            continue;
        }
        if (materials[i].alpha_mode == WR_ALPHA_MODE_BLEND) {
            continue;
        }

        ecs_entity_t parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);

        Walrus_Transform const *p_trans = ecs_get(it->world, parent, Walrus_Transform);
        mat4                    p_world;
        walrus_transform_compose(p_trans, p_world);

        Walrus_TransientBuffer const *weights = NULL;
        if (ecs_has(it->world, it->entities[i], Walrus_WeightResource)) {
            weights = &ecs_get(it->world, it->entities[i], Walrus_WeightResource)->weight_buffer;
        }
        walrus_material_submit(&materials[i]);
        walrus_deferred_renderer_submit_skinned_mesh(p_world, meshes[i].mesh, &skins[i].joint_buffer, weights);
    }
}

static void forward_submit_static_mesh(ecs_iter_t *it)
{
    Walrus_RenderMesh *meshes     = ecs_field(it, Walrus_RenderMesh, 1);
    Walrus_Material   *materials  = ecs_field(it, Walrus_Material, 2);
    Walrus_Transform  *transforms = ecs_field(it, Walrus_Transform, 3);

    for (i32 i = 0; i < it->count; ++i) {
        if (meshes[i].culled) {
            continue;
        }
        if (materials[i].alpha_mode != WR_ALPHA_MODE_BLEND) {
            continue;
        }

        mat4 world;
        walrus_transform_compose(&transforms[i], world);

        Walrus_TransientBuffer const *weights = NULL;
        if (ecs_has(it->world, it->entities[i], Walrus_WeightResource)) {
            weights = &ecs_get(it->world, it->entities[i], Walrus_WeightResource)->weight_buffer;
        }
        walrus_material_submit(&materials[i]);
        walrus_deferred_renderer_submit_mesh_ablend(world, meshes[i].mesh, weights);
    }
}

static void forward_submit_skinned_mesh(ecs_iter_t *it)
{
    Walrus_RenderMesh   *meshes    = ecs_field(it, Walrus_RenderMesh, 1);
    Walrus_Material     *materials = ecs_field(it, Walrus_Material, 2);
    Walrus_SkinResource *skins     = ecs_field(it, Walrus_SkinResource, 3);

    for (i32 i = 0; i < it->count; ++i) {
        if (meshes[i].culled) {
            continue;
        }
        if (materials[i].alpha_mode != WR_ALPHA_MODE_BLEND) {
            continue;
        }

        ecs_entity_t parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);

        Walrus_Transform const *p_trans = ecs_get(it->world, parent, Walrus_Transform);
        mat4                    p_world;
        walrus_transform_compose(p_trans, p_world);

        Walrus_TransientBuffer const *weights = NULL;
        if (ecs_has(it->world, it->entities[i], Walrus_WeightResource)) {
            weights = &ecs_get(it->world, it->entities[i], Walrus_WeightResource)->weight_buffer;
        }
        walrus_material_submit(&materials[i]);
        walrus_deferred_renderer_submit_skinned_mesh_ablend(p_world, meshes[i].mesh, &skins[i].joint_buffer, weights);
    }
}

Walrus_FramePipeline *walrus_deferred_pipeline_add(Walrus_FrameGraph *graph, char const *name)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ecs_id(deferred_submit_static_mesh) =
        ecs_system(ecs, {
                            .entity             = ecs_entity(ecs, {0}),
                            .query.filter.terms = {{.id = ecs_id(Walrus_RenderMesh)},
                                                   {.id = ecs_id(Walrus_Material)},
                                                   {.id = ecs_id(Walrus_Transform)},
                                                   {.id = ecs_id(Walrus_SkinResource), .oper = EcsNot}},
                            .callback           = deferred_submit_static_mesh,
                        });
    ECS_SYSTEM_DEFINE(ecs, deferred_submit_skinned_mesh, 0, Walrus_RenderMesh, Walrus_Material, Walrus_SkinResource);
    ecs_id(forward_submit_static_mesh) =
        ecs_system(ecs, {
                            .entity             = ecs_entity(ecs, {0}),
                            .query.filter.terms = {{.id = ecs_id(Walrus_RenderMesh)},
                                                   {.id = ecs_id(Walrus_Material)},
                                                   {.id = ecs_id(Walrus_Transform)},
                                                   {.id = ecs_id(Walrus_SkinResource), .oper = EcsNot}},
                            .callback           = forward_submit_static_mesh,
                        });
    ECS_SYSTEM_DEFINE(ecs, forward_submit_skinned_mesh, 0, Walrus_RenderMesh, Walrus_Material, Walrus_SkinResource);

    Walrus_FramePipeline *deferred_pipeline = walrus_fg_add_pipeline(graph, name);
    walrus_fg_add_node(deferred_pipeline, gbuffer_pass, "GBuffer");
    walrus_fg_add_node(deferred_pipeline, deferred_lighting_pass, "DeferredLighting");
    walrus_fg_add_node(deferred_pipeline, forward_lighting_pass, "ForwardLighting");

    return deferred_pipeline;
}
