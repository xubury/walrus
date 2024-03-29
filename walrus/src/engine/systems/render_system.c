#include <engine/systems/render_system.h>
#include <engine/systems/model_system.h>
#include <engine/systems/transform_system.h>
#include <engine/component.h>
#include <engine/systems/pipelines/culling_pipeline.h>
#include <engine/systems/pipelines/deferred_pipeline.h>
#include <engine/systems/pipelines/hdr_pipeline.h>

#include <engine/frame_graph.h>
#include <rhi/rhi.h>

#include <core/macro.h>
#include <core/memory.h>
#include <core/assert.h>
#include <core/math.h>

ECS_COMPONENT_DECLARE(Walrus_Renderer);
ECS_COMPONENT_DECLARE(Walrus_RenderMesh);
ECS_COMPONENT_DECLARE(Walrus_Material);
ECS_COMPONENT_DECLARE(Walrus_WeightResource);
ECS_COMPONENT_DECLARE(Walrus_SkinResource);

ECS_SYSTEM_DECLARE(weight_update);
ECS_SYSTEM_DECLARE(skin_update);
ECS_SYSTEM_DECLARE(renderer_run);

#define DEFERRED_LIGHTING_PASS "DeferredRenderPass"
#define CULLING_PASS           "CullingPass"
#define HDR_PASS               "HDRPass"

static void on_model_add(ecs_iter_t *it)
{
    Walrus_Transform *p_worlds = ecs_field(it, Walrus_Transform, 1);
    Walrus_ModelRef  *ref      = ecs_field(it, Walrus_ModelRef, 2);
    Walrus_Model     *model    = &ref->model;
    RenderSystem     *render   = it->ctx;

    ecs_entity_t  entity = it->entities[0];
    ecs_entity_t *skins  = walrus_new(ecs_entity_t, model->num_skins);
    for (u32 i = 0; i < model->num_skins; ++i) {
        skins[i] = ecs_new_w_pair(it->world, EcsChildOf, entity);
        ecs_set(it->world, skins[i], Walrus_SkinResource,
                {
                    .skin = &model->skins[i],
                });
    }

    for (u32 i = 0; i < model->num_nodes; ++i) {
        Walrus_ModelNode *node = &model->nodes[i];
        if (node->mesh) {
            Walrus_Transform const *world = &node->world_transform;

            ecs_entity_t weight = 0;
            if (node->mesh->num_weights > 0) {
                weight = ecs_new_w_pair(it->world, EcsChildOf, entity);
                ecs_set(it->world, weight, Walrus_WeightResource,
                        {
                            .node = node,
                        });
            }

            for (u32 j = 0; j < node->mesh->num_primitives; ++j) {
                Walrus_Material *material = node->mesh->primitives[j].material;
                ecs_entity_t     mesh     = ecs_new_w_pair(it->world, EcsChildOf, entity);
                ecs_set_name(it->world, mesh, node->mesh->name);
                if (node->skin) {
                    u32 skin_id = node->skin - &model->skins[0];
                    ecs_add_pair(it->world, mesh, EcsIsA, skins[skin_id]);
                }
                if (weight) {
                    ecs_add_pair(it->world, mesh, EcsIsA, weight);
                }

                ecs_set(it->world, mesh, Walrus_RenderMesh, {.mesh = &node->mesh->primitives[j], .culled = false});
                ecs_set_ptr(it->world, mesh, Walrus_Material, material ? material : &render->default_material);

                walrus_transform_mul(&p_worlds[0], world, ecs_get_mut(it->world, mesh, Walrus_Transform));
                ecs_modified(it->world, mesh, Walrus_Transform);
            }
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

static void renderer_run(ecs_iter_t *it)
{
    RenderSystem    *render    = it->param;
    Walrus_Renderer *renderers = ecs_field(it, Walrus_Renderer, 1);
    Walrus_Camera   *cameras   = ecs_field(it, Walrus_Camera, 2);

    for (i32 i = 0; i < it->count; ++i) {
        if (!renderers[i].active) {
            continue;
        }
        u16 view_slot = 0;

        walrus_fg_write(&render->render_graph, "ColorBuffer", walrus_rhi_get_texture(render->backrt, 0).id);
        walrus_fg_write(&render->render_graph, "DepthBuffer", walrus_rhi_get_texture(render->backrt, 1).id);
        walrus_fg_write(&render->render_graph, "BackRT", render->backrt.id);
        walrus_fg_write_ptr(&render->render_graph, "Renderer", &renderers[i]);
        walrus_fg_write_ptr(&render->render_graph, "Camera", &cameras[i]);
        walrus_fg_write_ptr(&render->render_graph, "ViewSlot", &view_slot);
        walrus_fg_execute(&render->render_graph, HDR_PASS);
        /* walrus_rhi_set_debug(WR_RHI_DEBUG_STATS); */
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
        Walrus_Model const *model  = &ecs_get(it->world, parent, Walrus_ModelRef)->model;
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
        Walrus_Model const *model  = &ecs_get(it->world, parent, Walrus_ModelRef)->model;
        walrus_rhi_alloc_transient_buffer(&skins[i].joint_buffer, skins[i].skin->num_joints, sizeof(mat4),
                                          walrus_max(walrus_rhi_get_caps()->ssbo_align, alignof(mat4)));

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

static void render_system_init(Walrus_System *sys)
{
    RenderSystem *render = poly_cast(sys, RenderSystem);
    ecs_world_t  *ecs    = sys->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_Renderer);
    ECS_COMPONENT_DEFINE(ecs, Walrus_RenderMesh);
    ECS_COMPONENT_DEFINE(ecs, Walrus_Material);
    ECS_COMPONENT_DEFINE(ecs, Walrus_WeightResource);
    ECS_COMPONENT_DEFINE(ecs, Walrus_SkinResource);

    ecs_observer(ecs, {.events       = {EcsOnAdd},
                       .entity       = ecs_entity(ecs, {0}),
                       .callback     = on_model_add,
                       .ctx          = render,
                       .filter.terms = {{.id = ecs_id(Walrus_Transform)},
                                        {.id = ecs_id(Walrus_ModelRef)},
                                        {.id = ecs_id(Walrus_ModelRef), .src.flags = EcsSelf, .oper = EcsNot}}});
    ecs_observer(ecs, {.events       = {EcsOnRemove},
                       .entity       = ecs_entity(ecs, {0}),
                       .callback     = on_model_remove,
                       .filter.terms = {{.id = ecs_id(Walrus_ModelRef)},
                                        {.id = ecs_id(Walrus_ModelRef), .src.flags = EcsSelf, .oper = EcsNot}}});

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

    ECS_SYSTEM_DEFINE(ecs, renderer_run, 0, Walrus_Renderer, Walrus_Camera);

    walrus_renderer_init();

    u64 flags =
        (u64)(walrus_u32cnttz(walrus_rhi_get_mssa()) + 1) << WR_RHI_TEXTURE_RT_MSAA_SHIFT | WR_RHI_SAMPLER_UVW_CLAMP;

    Walrus_Attachment attachments[2] = {0};

    attachments[0].handle = walrus_rhi_create_texture(
        &(Walrus_TextureCreateInfo){
            .ratio = WR_RHI_RATIO_EQUAL, .format = WR_RHI_FORMAT_RGBA8, .num_mipmaps = 1, .flags = flags},
        NULL);
    attachments[0].access = WR_RHI_ACCESS_WRITE;
    attachments[1].handle = walrus_rhi_create_texture(
        &(Walrus_TextureCreateInfo){
            .ratio = WR_RHI_RATIO_EQUAL, .format = WR_RHI_FORMAT_DEPTH24, .num_mipmaps = 1, .flags = flags},
        NULL);
    attachments[1].access = WR_RHI_ACCESS_WRITE;

    render->backrt = walrus_rhi_create_framebuffer(attachments, walrus_count_of(attachments));

    walrus_fg_init(&render->render_graph);

    walrus_fg_write(&render->render_graph, "ColorBuffer", walrus_rhi_get_texture(render->backrt, 0).id);
    walrus_fg_write(&render->render_graph, "DepthBuffer", walrus_rhi_get_texture(render->backrt, 1).id);
    walrus_fg_write(&render->render_graph, "BackRT", render->backrt.id);

    Walrus_FramePipeline *culling_pipeline = walrus_culling_pipeline_add(&render->render_graph, CULLING_PASS);
    Walrus_FramePipeline *deferred_pipeline =
        walrus_deferred_pipeline_add(&render->render_graph, DEFERRED_LIGHTING_PASS);
    Walrus_FramePipeline *hdr_pipeline = walrus_hdr_pipeline_add(&render->render_graph, HDR_PASS);

    walrus_fg_connect_pipeline(culling_pipeline, deferred_pipeline);
    walrus_fg_connect_pipeline(deferred_pipeline, hdr_pipeline);

    walrus_fg_compile(&render->render_graph);

    walrus_model_material_init_default(&render->default_material);
}

static void render_system_shutdown(Walrus_System *sys)
{
    RenderSystem *render = poly_cast(sys, RenderSystem);
    walrus_fg_shutdown(&render->render_graph);
    walrus_renderer_shutdown();
}

static void render_system_render(Walrus_System *sys)
{
    ecs_world_t  *ecs    = sys->ecs;
    RenderSystem *render = poly_cast(sys, RenderSystem);

    ecs_run(ecs, ecs_id(weight_update), 0, NULL);
    ecs_run(ecs, ecs_id(skin_update), 0, NULL);
    ecs_run(ecs, ecs_id(renderer_run), 0, render);
}

POLY_DEFINE_DERIVED(Walrus_System, RenderSystem, render_system_create, POLY_IMPL(on_system_init, render_system_init),
                    POLY_IMPL(on_system_shutdown, render_system_shutdown),
                    POLY_IMPL(on_system_render, render_system_render))
