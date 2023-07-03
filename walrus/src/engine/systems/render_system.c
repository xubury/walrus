#include <engine/systems/render_system.h>
#include <engine/systems/animator_system.h>
#include <engine/systems/model_system.h>
#include <engine/systems/transform_system.h>
#include <engine/systems/pipelines/culling_pipeline.h>
#include <engine/systems/pipelines/deferred_pipeline.h>

#include <engine/frame_graph.h>
#include <engine/engine.h>

#include <core/macro.h>
#include <core/memory.h>
#include <core/assert.h>
#include <core/math.h>

static Walrus_FrameGraph s_render_graph;

ECS_COMPONENT_DECLARE(Walrus_DeferredRenderer);
ECS_COMPONENT_DECLARE(Walrus_RenderMesh);
ECS_COMPONENT_DECLARE(Walrus_WeightResource);
ECS_COMPONENT_DECLARE(Walrus_SkinResource);

ECS_SYSTEM_DECLARE(weight_update);
ECS_SYSTEM_DECLARE(skin_update);
ECS_SYSTEM_DECLARE(deferred_renderer_run);

#define DEFERRED_LIGHTING_PASS "DeferredRenderPass"
#define CULLING_PASS           "CullingPass"

static void on_model_add(ecs_iter_t *it)
{
    Walrus_Model *model = ecs_field(it, Walrus_Model, 2);

    ecs_entity_t *skins = walrus_new(ecs_entity_t, model->num_skins);
    for (u32 i = 0; i < model->num_skins; ++i) {
        skins[i] = ecs_new_w_pair(it->world, EcsChildOf, it->entities[0]);
        ecs_set(it->world, skins[i], Walrus_SkinResource,
                {
                    .skin = &model->skins[i],
                });
    }

    for (u32 i = 0; i < model->num_nodes; ++i) {
        Walrus_ModelNode *node = &model->nodes[i];
        ecs_entity_t      mesh = ecs_new_w_pair(it->world, EcsChildOf, it->entities[0]);
        if (node->mesh) {
            Walrus_Transform const *transform = &node->world_transform;

            if (node->mesh->num_weights > 0) {
                ecs_entity_t weight = ecs_new_w_pair(it->world, EcsChildOf, it->entities[0]);
                ecs_set(it->world, weight, Walrus_WeightResource,
                        {
                            .node = node,
                        });
                ecs_add_pair(it->world, mesh, EcsIsA, weight);
            }

            if (node->skin) {
                u32 skin_id = node->skin - &model->skins[0];
                ecs_add_pair(it->world, mesh, EcsIsA, skins[skin_id]);
            }

            ecs_set(it->world, mesh, Walrus_RenderMesh, {.mesh = node->mesh, .culled = false});
            ecs_set(it->world, mesh, Walrus_Transform, {0});
            ecs_set(it->world, mesh, Walrus_LocalTransform,
                    {.trans = {transform->trans[0], transform->trans[1], transform->trans[2]},
                     .rot   = {transform->rot[0], transform->rot[1], transform->rot[2], transform->rot[3]},
                     .scale = {transform->scale[0], transform->scale[1], transform->scale[2]}});
        }
    }
    walrus_free(skins);
}

static void on_model_remove(ecs_iter_t *it)
{
    ecs_entity_t entity = it->entities[0];
    {
        ecs_filter_t *f = ecs_filter_init(
            it->world,
            &(ecs_filter_desc_t){.terms = {{.id = ecs_id(Walrus_RenderMesh)}, {.id = ecs_pair(EcsChildOf, entity)}}});
        ecs_iter_t it_f = ecs_filter_iter(it->world, f);
        while (ecs_filter_next(&it_f)) {
            for (i32 i = 0; i < it_f.count; ++i) {
                ecs_delete(it->world, it_f.entities[i]);
            }
        }
        ecs_filter_fini(f);
    }
    {
        ecs_filter_t *f = ecs_filter_init(
            it->world, &(ecs_filter_desc_t){.terms = {{.id = ecs_id(Walrus_WeightResource), .src.flags = EcsSelf},
                                                      {.id = ecs_pair(EcsChildOf, entity)}}});
        ecs_iter_t it_f = ecs_filter_iter(it->world, f);
        while (ecs_filter_next(&it_f)) {
            for (i32 i = 0; i < it_f.count; ++i) {
                ecs_delete(it->world, it_f.entities[i]);
            }
        }
        ecs_filter_fini(f);
    }
    {
        ecs_filter_t *f = ecs_filter_init(
            it->world, &(ecs_filter_desc_t){.terms = {{.id = ecs_id(Walrus_SkinResource), .src.flags = EcsSelf},
                                                      {.id = ecs_pair(EcsChildOf, entity)}}});
        ecs_iter_t it_f = ecs_filter_iter(it->world, f);
        while (ecs_filter_next(&it_f)) {
            for (i32 i = 0; i < it_f.count; ++i) {
                ecs_delete(it->world, it_f.entities[i]);
            }
        }
        ecs_filter_fini(f);
    }
}

static void transform_bound(mat4 const m, vec3 const min, vec3 const max, vec3 res_min, vec3 res_max)
{
    vec3 center;
    glm_vec3_add((f32 *)min, (f32 *)max, center);
    glm_vec3_scale(center, 0.5f, center);
    vec3 extends;
    glm_vec3_sub((f32 *)max, center, extends);

    vec3 new_center;
    glm_mat4_mulv3((vec4 *)m, center, 1.0, new_center);
    vec3 new_extends = {
        fabs(extends[0] * m[0][0]) + fabs(extends[1] * m[1][0]) + fabs(extends[2] * m[2][0]),
        fabs(extends[0] * m[0][1]) + fabs(extends[1] * m[1][1]) + fabs(extends[2] * m[2][1]),
        fabs(extends[0] * m[0][2]) + fabs(extends[1] * m[1][2]) + fabs(extends[2] * m[2][2]),
    };
    glm_vec3_sub(new_center, new_extends, res_min);
    glm_vec3_add(new_center, new_extends, res_max);
}

static void deferred_renderer_run(ecs_iter_t *it)
{
    Walrus_DeferredRenderer *renderers = ecs_field(it, Walrus_DeferredRenderer, 1);
    Walrus_Camera           *cameras   = ecs_field(it, Walrus_Camera, 2);

    for (i32 i = 0; i < it->count; ++i) {
        if (!renderers[i].active) {
            continue;
        }
        walrus_deferred_renderer_set_camera(&renderers[i], &cameras[i]);
        walrus_deferred_renderer_start_record(&renderers[i]);
        walrus_fg_write_ptr(&s_render_graph, "DeferredRenderer", &renderers[i]);
        walrus_fg_write_ptr(&s_render_graph, "Camera", &cameras[i]);
        walrus_fg_execute(&s_render_graph, DEFERRED_LIGHTING_PASS);
        char buffer[255];
        walrus_deferred_renderer_log_stats(&renderers[i], buffer, 255);
        walrus_trace(buffer);
        walrus_deferred_renderer_end_record(&renderers[i]);
    }
    walrus_rhi_touch(0);
}

static void weight_update(ecs_iter_t *it)
{
    Walrus_WeightResource *weights = ecs_field(it, Walrus_WeightResource, 1);
    for (i32 i = 0; i < it->count; ++i) {
        walrus_rhi_alloc_transient_buffer(&weights[i].weight_buffer, weights[i].node->mesh->num_weights, sizeof(f32),
                                          walrus_rhi_get_caps()->ssbo_align);

        ecs_entity_t        parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);
        Walrus_Model const *model  = ecs_get(it->world, parent, Walrus_Model);
        if (ecs_has(it->world, parent, Walrus_Animator)) {
            Walrus_Animator const *animator = ecs_get(it->world, parent, Walrus_Animator);
            walrus_animator_weights(animator, model, weights[i].node, (f32 *)weights[i].weight_buffer.data);
        }
        else {
            memcpy(weights[i].weight_buffer.data, weights[i].node->mesh->weights,
                   sizeof(f32) * weights[i].node->mesh->num_weights);
        }
    }
}

static void skin_update(ecs_iter_t *it)
{
    Walrus_SkinResource *skins = ecs_field(it, Walrus_SkinResource, 1);
    for (i32 i = 0; i < it->count; ++i) {
        ecs_entity_t        parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);
        Walrus_Model const *model  = ecs_get(it->world, parent, Walrus_Model);
        walrus_rhi_alloc_transient_buffer(&skins[i].joint_buffer, skins[i].skin->num_joints, sizeof(mat4),
                                          walrus_max(walrus_rhi_get_caps()->ssbo_align, 16));

        Walrus_ModelSkin *skin = skins[i].skin;

        mat4 *skin_matrices = (mat4 *)skins[i].joint_buffer.data;

        glm_vec3_copy((vec3){FLT_MAX, FLT_MAX, FLT_MAX}, skins[i].min);
        glm_vec3_copy((vec3){-FLT_MAX, -FLT_MAX, -FLT_MAX}, skins[i].max);
        for (u32 j = 0; j < skin->num_joints; ++j) {
            mat4 m;
            if (ecs_has(it->world, parent, Walrus_Animator)) {
                Walrus_Animator const *animator = ecs_get(it->world, parent, Walrus_Animator);
                walrus_animator_transform(animator, model, skin->joints[j].node, m);
                glm_mat4_mul(m, skin->joints[j].inverse_bind_matrix, skin_matrices[j]);
            }
            else {
                walrus_transform_compose(&skin->joints[j].node->world_transform, m);
                glm_mat4_mul(m, skin->joints[j].inverse_bind_matrix, skin_matrices[j]);
            }
            vec3 min, max;
            transform_bound(m, skin->joints[j].min, skin->joints[j].max, min, max);
            glm_vec3_minv(min, skins[i].min, skins[i].min);
            glm_vec3_maxv(max, skins[i].max, skins[i].max);
        }
    }
}

void walrus_render_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_DeferredRenderer);
    ECS_COMPONENT_DEFINE(ecs, Walrus_RenderMesh);
    ECS_COMPONENT_DEFINE(ecs, Walrus_WeightResource);
    ECS_COMPONENT_DEFINE(ecs, Walrus_SkinResource);

    // TODO: filter not self?
    ECS_OBSERVER(ecs, on_model_add, EcsOnSet, Walrus_Transform, Walrus_Model);
    ECS_OBSERVER(ecs, on_model_remove, EcsOnRemove, Walrus_Model);

    ecs_id(skin_update)   = ecs_system(ecs, {
                                                .entity = ecs_entity(ecs, {0}),
                                                .query.filter.terms =
                                                  {
                                                      {.id = ecs_id(Walrus_SkinResource), .src.flags = EcsSelf},
                                                  },
                                                .callback = skin_update,
                                          });
    ecs_id(weight_update) = ecs_system(ecs, {
                                                .entity = ecs_entity(ecs, {0}),
                                                .query.filter.terms =
                                                    {
                                                        {.id = ecs_id(Walrus_WeightResource), .src.flags = EcsSelf},
                                                    },
                                                .callback = weight_update,
                                            });

    ECS_SYSTEM_DEFINE(ecs, deferred_renderer_run, 0, Walrus_DeferredRenderer, Walrus_Camera);

    walrus_deferred_renderer_init_uniforms();

    walrus_fg_init(&s_render_graph);

    Walrus_FramePipeline *culling_pipeline  = walrus_culling_pipeline_add(&s_render_graph, CULLING_PASS);
    Walrus_FramePipeline *deferred_pipeline = walrus_deferred_pipeline_add(&s_render_graph, DEFERRED_LIGHTING_PASS);

    walrus_fg_connect_pipeline(culling_pipeline, deferred_pipeline);

    walrus_fg_compile(&s_render_graph);
}

void walrus_render_system_shutdown(void)
{
    walrus_fg_shutdown(&s_render_graph);
}

void walrus_render_system_render(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    ecs_run(ecs, ecs_id(weight_update), 0, NULL);
    ecs_run(ecs, ecs_id(skin_update), 0, NULL);
    ecs_run(ecs, ecs_id(deferred_renderer_run), 0, NULL);
}
