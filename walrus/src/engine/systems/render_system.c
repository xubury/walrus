#include <engine/systems/render_system.h>
#include <engine/systems/animator_system.h>
#include <engine/systems/model_system.h>
#include <engine/systems/transform_system.h>
#include <engine/frame_graph.h>
#include <engine/engine.h>
#include <core/macro.h>
#include <core/memory.h>
#include <core/assert.h>

static Walrus_FrameGraph s_render_graph;

ECS_COMPONENT_DECLARE(Walrus_DeferredRenderer);
ECS_COMPONENT_DECLARE(Walrus_StaticMesh);
ECS_COMPONENT_DECLARE(Walrus_SkinnedMesh);
ECS_COMPONENT_DECLARE(Walrus_MeshResource);
ECS_COMPONENT_DECLARE(Walrus_SkinResource);

ECS_SYSTEM_DECLARE(mesh_update);
ECS_SYSTEM_DECLARE(skin_update);
ECS_SYSTEM_DECLARE(deferred_submit_static_mesh);
ECS_SYSTEM_DECLARE(deferred_submit_skinned_mesh);
ECS_SYSTEM_DECLARE(cull_test_static_mesh);
ECS_SYSTEM_DECLARE(cull_test_skinned_mesh);
ECS_SYSTEM_DECLARE(deferred_renderer_run);

#define DEFERRED_RENDERER_PASS "DeferredRenderPass"
#define CULLING_PASS           "CullingPass"

static void on_model_add(ecs_iter_t *it)
{
    Walrus_Model *model = ecs_field(it, Walrus_Model, 2);

    ecs_entity_t *meshes = walrus_new(ecs_entity_t, model->num_meshes);
    ecs_entity_t *skins  = walrus_new(ecs_entity_t, model->num_skins);
    for (u32 i = 0; i < model->num_meshes; ++i) {
        meshes[i] = ecs_new_w_pair(it->world, EcsChildOf, it->entities[0]);
        ecs_set(it->world, meshes[i], Walrus_MeshResource, {.mesh = &model->meshes[i]});
    }
    for (u32 i = 0; i < model->num_skins; ++i) {
        skins[i] = ecs_new_w_pair(it->world, EcsChildOf, it->entities[0]);
        ecs_set(it->world, skins[i], Walrus_SkinResource, {.skin = &model->skins[i]});
    }

    for (u32 i = 0; i < model->num_nodes; ++i) {
        Walrus_ModelNode *node = &model->nodes[i];
        ecs_entity_t      mesh = ecs_new_w_pair(it->world, EcsChildOf, it->entities[0]);
        if (node->mesh) {
            Walrus_Transform const *transform = &node->world_transform;

            u32 mesh_id = node->mesh - &model->meshes[0];
            ecs_add_pair(it->world, mesh, EcsDependsOn, meshes[mesh_id]);
            if (node->skin) {
                ecs_set(it->world, mesh, Walrus_SkinnedMesh, {.mesh = node->mesh, .node = node, .culled = false});
                u32 skin_id = node->skin - &model->skins[0];
                ecs_add_pair(it->world, mesh, EcsDependsOn, skins[skin_id]);
            }
            else {
                ecs_set(it->world, mesh, Walrus_StaticMesh, {.mesh = node->mesh, .node = node, .culled = false});
            }
            ecs_set(it->world, mesh, Walrus_Transform, {0});
            ecs_set(it->world, mesh, Walrus_LocalTransform,
                    {.trans = {transform->trans[0], transform->trans[1], transform->trans[2]},
                     .rot   = {transform->rot[0], transform->rot[1], transform->rot[2], transform->rot[3]},
                     .scale = {transform->scale[0], transform->scale[1], transform->scale[2]}});
        }
    }
    walrus_free(meshes);
    walrus_free(skins);
}

static void remove_dependency(ecs_world_t *world, ecs_entity_t entity)
{
    ecs_filter_t *ff = ecs_filter_init(world, &(ecs_filter_desc_t){.terms = {{.id = ecs_pair(EcsDependsOn, entity)}}});
    ecs_iter_t    it_ff = ecs_filter_iter(world, ff);
    while (ecs_filter_next(&it_ff)) {
        for (i32 i = 0; i < it_ff.count; ++i) {
            ecs_delete(world, it_ff.entities[i]);
        }
    }
    ecs_filter_fini(ff);
}

static void on_model_remove(ecs_iter_t *it)
{
    ecs_entity_t  entity = it->entities[0];
    ecs_filter_t *f    = ecs_filter_init(it->world, &(ecs_filter_desc_t){.terms = {{.id = ecs_id(Walrus_SkinResource)},
                                                                                   {.id = ecs_pair(EcsChildOf, entity)}}});
    ecs_iter_t    it_f = ecs_filter_iter(it->world, f);
    while (ecs_filter_next(&it_f)) {
        for (i32 i = 0; i < it_f.count; ++i) {
            remove_dependency(it->world, it_f.entities[i]);
            ecs_delete(it->world, it_f.entities[i]);
        }
    }
    ecs_filter_fini(f);

    f = ecs_filter_init(it->world, &(ecs_filter_desc_t){.terms = {{.id = ecs_id(Walrus_MeshResource)},
                                                                  {.id = ecs_pair(EcsChildOf, entity)}}});

    it_f = ecs_filter_iter(it->world, f);
    while (ecs_filter_next(&it_f)) {
        for (i32 i = 0; i < it_f.count; ++i) {
            remove_dependency(it->world, it_f.entities[i]);
            ecs_delete(it->world, it_f.entities[i]);
        }
    }
    ecs_filter_fini(f);
}

static void deferred_submit_static_mesh(ecs_iter_t *it)
{
    Walrus_StaticMesh       *meshes     = ecs_field(it, Walrus_StaticMesh, 1);
    Walrus_Transform        *transforms = ecs_field(it, Walrus_Transform, 2);
    Walrus_DeferredRenderer *renderer   = it->param;

    for (i32 i = 0; i < it->count; ++i) {
        if (meshes[i].culled) {
            continue;
        }

        mat4 world;
        walrus_transform_compose(&transforms[i], world);

        ecs_entity_t res_mesh =
            ecs_get_target_for_id(it->real_world, it->entities[i], EcsDependsOn, ecs_id(Walrus_MeshResource));
        walrus_deferred_renderer_submit_mesh(renderer, world, &meshes[i],
                                             ecs_get(it->world, res_mesh, Walrus_MeshResource)->weight_buffer);
    }
}
static void deferred_submit_skinned_mesh(ecs_iter_t *it)
{
    Walrus_SkinnedMesh      *meshes   = ecs_field(it, Walrus_SkinnedMesh, 1);
    Walrus_DeferredRenderer *renderer = it->param;

    for (i32 i = 0; i < it->count; ++i) {
        if (meshes[i].culled) {
            continue;
        }

        ecs_entity_t parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);

        Walrus_Transform const *p_trans = ecs_get(it->world, parent, Walrus_Transform);

        mat4 p_world;
        walrus_transform_compose(p_trans, p_world);
        ecs_entity_t res_skin =
            ecs_get_target_for_id(it->real_world, it->entities[i], EcsDependsOn, ecs_id(Walrus_SkinResource));
        ecs_entity_t res_mesh =
            ecs_get_target_for_id(it->real_world, it->entities[i], EcsDependsOn, ecs_id(Walrus_MeshResource));
        walrus_deferred_renderer_submit_skinned_mesh(renderer, p_world, &meshes[i],
                                                     ecs_get(it->world, res_skin, Walrus_SkinResource)->joint_buffer,
                                                     ecs_get(it->world, res_mesh, Walrus_MeshResource)->weight_buffer);
    }
}

static void cull_test_static_mesh(ecs_iter_t *it)
{
    Walrus_StaticMesh *meshes     = ecs_field(it, Walrus_StaticMesh, 1);
    Walrus_Transform  *transforms = ecs_field(it, Walrus_Transform, 2);
    Walrus_Camera     *camera     = it->param;

    for (i32 i = 0; i < it->count; ++i) {
        mat4 world;
        walrus_transform_compose(&transforms[i], world);

        meshes[i].culled = false;

        if (!walrus_camera_frustum_cull_test(camera, world, meshes[i].mesh->min, meshes[i].mesh->max)) {
            meshes[i].culled = true;
            continue;
        }
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

static void cull_test_skinned_mesh(ecs_iter_t *it)
{
    Walrus_SkinnedMesh *meshes = ecs_field(it, Walrus_SkinnedMesh, 1);
    Walrus_Camera      *camera = it->param;

    for (i32 i = 0; i < it->count; ++i) {
        ecs_entity_t parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);

        Walrus_Transform const *p_trans = ecs_get(it->world, parent, Walrus_Transform);

        mat4 p_world;
        walrus_transform_compose(p_trans, p_world);

        meshes[i].culled = false;

        ecs_entity_t res_skin =
            ecs_get_target_for_id(it->real_world, it->entities[i], EcsDependsOn, ecs_id(Walrus_SkinResource));
        Walrus_SkinResource const *skin = ecs_get(it->world, res_skin, Walrus_SkinResource);
        if (!walrus_camera_frustum_cull_test(camera, p_world, skin->min, skin->max)) {
            meshes[i].culled = true;
            continue;
        }
    }
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
        walrus_fg_execute(&s_render_graph, DEFERRED_RENDERER_PASS);
        char buffer[255];
        walrus_deferred_renderer_log_stats(&renderers[i], buffer, 255);
        walrus_trace(buffer);
        walrus_deferred_renderer_end_record(&renderers[i]);
    }
    walrus_rhi_touch(0);
}

static void culling_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    ecs_world_t   *ecs    = walrus_engine_vars()->ecs;
    Walrus_Camera *camera = walrus_fg_read_ptr(graph, "Camera");

    ecs_run(ecs, ecs_id(cull_test_static_mesh), 0, camera);
    ecs_run(ecs, ecs_id(cull_test_skinned_mesh), 0, camera);

    /* walrus_trace("render index: %d name: %s", node->index, node->name); */
}

static void gbuffer_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    Walrus_DeferredRenderer *renderer = walrus_fg_read_ptr(graph, "DeferredRenderer");
    Walrus_FramebufferHandle gbuffer  = renderer->gbuffer;
    walrus_fg_write(graph, "GBuffer", gbuffer.id);

    ecs_run(ecs, ecs_id(deferred_submit_static_mesh), 0, renderer);
    ecs_run(ecs, ecs_id(deferred_submit_skinned_mesh), 0, renderer);

    /* walrus_trace("render index: %d name: %s gbuffer: %d", node->index, node->name, gbuffer.id); */
}

static void deferred_lighting_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    Walrus_FramebufferHandle gbuffer = {walrus_fg_read(graph, "GBuffer")};

    /* walrus_trace("render index: %d name: %s gbuffer: %d", node->index, node->name, gbuffer.id); */
}

static void mesh_update(ecs_iter_t *it)
{
    Walrus_MeshResource *resource = ecs_field(it, Walrus_MeshResource, 1);
    for (i32 i = 0; i < it->count; ++i) {
        if (resource[i].mesh->num_weights > 0) {
            walrus_rhi_alloc_transient_buffer(&resource[i].weight_buffer, resource[i].mesh->num_weights, sizeof(f32));
        }
        else {
            resource[i].weight_buffer.handle.id = WR_INVALID_HANDLE;
        }
        memcpy(resource[i].weight_buffer.data, resource[i].mesh->weights, sizeof(f32) * resource[i].mesh->num_weights);
    }
}

static void skin_update(ecs_iter_t *it)
{
    Walrus_SkinResource *resource = ecs_field(it, Walrus_SkinResource, 1);
    for (i32 i = 0; i < it->count; ++i) {
        ecs_entity_t        parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);
        Walrus_Model const *model  = ecs_get(it->world, parent, Walrus_Model);
        walrus_rhi_alloc_transient_buffer(&resource[i].joint_buffer, resource[i].skin->num_joints, sizeof(mat4));

        Walrus_ModelSkin *skin = resource[i].skin;

        mat4 *m = (mat4 *)resource[i].joint_buffer.data;
        if (ecs_has(it->world, parent, Walrus_Animator)) {
            Walrus_Animator const *animator = ecs_get(it->world, parent, Walrus_Animator);
            for (u32 i = 0; i < skin->num_joints; ++i) {
                walrus_animator_transform(animator, model, skin->joints[i].node, m[i]);
                glm_mat4_mul(m[i], skin->joints[i].inverse_bind_matrix, m[i]);
            }
        }
        else {
            for (u32 i = 0; i < skin->num_joints; ++i) {
                walrus_transform_compose(&skin->joints[i].node->world_transform, m[i]);
                glm_mat4_mul(m[i], skin->joints[i].inverse_bind_matrix, m[i]);
            }
        }
        glm_vec3_copy((vec3){FLT_MAX, FLT_MAX, FLT_MAX}, resource[i].min);
        glm_vec3_copy((vec3){-FLT_MAX, -FLT_MAX, -FLT_MAX}, resource[i].max);
        for (u32 j = 0; j < skin->num_joints; ++j) {
            mat4 m;
            if (ecs_has(it->world, parent, Walrus_Animator)) {
                Walrus_Animator const *animator = ecs_get(it->world, parent, Walrus_Animator);
                walrus_animator_transform(animator, model, skin->joints[j].node, m);
            }
            else {
                walrus_transform_compose(&skin->joints[j].node->world_transform, m);
            }
            vec3 min, max;
            if (skin->joints[j].min[0] == FLT_MAX || skin->joints[j].max[0] == -FLT_MAX) {
                continue;
            }
            transform_bound(m, skin->joints[j].min, skin->joints[j].max, min, max);
            glm_vec3_minv(min, resource[i].min, resource[i].min);
            glm_vec3_maxv(max, resource[i].max, resource[i].max);
        }
    }
}

void walrus_render_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_DeferredRenderer);
    ECS_COMPONENT_DEFINE(ecs, Walrus_StaticMesh);
    ECS_COMPONENT_DEFINE(ecs, Walrus_SkinnedMesh);
    ECS_COMPONENT_DEFINE(ecs, Walrus_MeshResource);
    ECS_COMPONENT_DEFINE(ecs, Walrus_SkinResource);

    ECS_OBSERVER(ecs, on_model_add, EcsOnSet, Walrus_Transform, Walrus_Model);
    ECS_OBSERVER(ecs, on_model_remove, EcsOnRemove, Walrus_Model);

    ECS_SYSTEM_DEFINE(ecs, mesh_update, 0, Walrus_MeshResource);
    ECS_SYSTEM_DEFINE(ecs, skin_update, 0, Walrus_SkinResource);
    ECS_SYSTEM_DEFINE(ecs, deferred_submit_static_mesh, 0, Walrus_StaticMesh, Walrus_Transform);
    ECS_SYSTEM_DEFINE(ecs, deferred_submit_skinned_mesh, 0, Walrus_SkinnedMesh);
    ECS_SYSTEM_DEFINE(ecs, deferred_renderer_run, 0, Walrus_DeferredRenderer, Walrus_Camera);
    ECS_SYSTEM_DEFINE(ecs, cull_test_static_mesh, 0, Walrus_StaticMesh, Walrus_Transform);
    ECS_SYSTEM_DEFINE(ecs, cull_test_skinned_mesh, 0, Walrus_SkinnedMesh);

    walrus_deferred_renderer_init_uniforms();

    walrus_fg_init(&s_render_graph);

    Walrus_FramePipeline *dummy_pipeline = walrus_fg_add_pipeline(&s_render_graph, "dummy");

    Walrus_FramePipeline *culling_pipeline = walrus_fg_add_pipeline(&s_render_graph, CULLING_PASS);
    walrus_fg_add_node(culling_pipeline, culling_pass, "Culling");

    Walrus_FramePipeline *deferred_pipeline = walrus_fg_add_pipeline(&s_render_graph, DEFERRED_RENDERER_PASS);
    walrus_fg_add_node(deferred_pipeline, gbuffer_pass, "GBuffer");
    walrus_fg_add_node(deferred_pipeline, deferred_lighting_pass, "DeferredLighting");

    walrus_fg_connect_pipeline(culling_pipeline, deferred_pipeline);
    walrus_fg_connect_pipeline(dummy_pipeline, culling_pipeline);

    walrus_fg_compile(&s_render_graph);
}

void walrus_render_system_shutdown(void)
{
    walrus_fg_shutdown(&s_render_graph);
}

void walrus_render_system_render(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    ecs_run(ecs, ecs_id(mesh_update), 0, NULL);
    ecs_run(ecs, ecs_id(skin_update), 0, NULL);
    ecs_run(ecs, ecs_id(deferred_renderer_run), 0, NULL);
}
