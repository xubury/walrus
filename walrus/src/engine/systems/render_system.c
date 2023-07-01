#include <engine/systems/render_system.h>
#include <engine/systems/animator_system.h>
#include <engine/systems/model_system.h>
#include <engine/systems/transform_system.h>
#include <engine/frame_graph.h>
#include <engine/engine.h>
#include <core/macro.h>

static Walrus_FrameGraph s_render_graph;

ECS_COMPONENT_DECLARE(Walrus_DeferredRenderer);
ECS_COMPONENT_DECLARE(Walrus_StaticMesh);
ECS_COMPONENT_DECLARE(Walrus_SkinnedMesh);

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

    for (u32 i = 0; i < model->num_nodes; ++i) {
        Walrus_ModelNode *node = &model->nodes[i];
        ecs_entity_t      mesh = ecs_new_w_pair(it->world, EcsChildOf, it->entities[0]);
        if (node->mesh) {
            Walrus_Transform const *transform = &node->world_transform;
            if (node->skin) {
                ecs_set(it->world, mesh, Walrus_SkinnedMesh,
                        {.mesh = node->mesh, .skin = node->skin, .node = node, .culled = false});
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
}

static void on_model_remove(ecs_iter_t *it)
{
    ecs_entity_t  entity = it->entities[0];
    ecs_filter_t *f      = ecs_filter_init(it->world, &(ecs_filter_desc_t){.terms = {{.id = ecs_id(Walrus_SkinnedMesh)},
                                                                                     {.id = ecs_pair(EcsChildOf, entity)}}});
    ecs_iter_t    it_f   = ecs_filter_iter(it->world, f);
    while (ecs_filter_next(&it_f)) {
        for (i32 i = 0; i < it_f.count; ++i) {
            ecs_delete(it->world, it_f.entities[i]);
        }
    }
    ecs_filter_fini(f);

    f = ecs_filter_init(it->world, &(ecs_filter_desc_t){.terms = {{.id = ecs_id(Walrus_StaticMesh)},
                                                                  {.id = ecs_pair(EcsChildOf, entity)}}});

    it_f = ecs_filter_iter(it->world, f);
    while (ecs_filter_next(&it_f)) {
        for (i32 i = 0; i < it_f.count; ++i) {
            ecs_delete(it->world, it_f.entities[i]);
        }
    }
    ecs_filter_fini(f);
}

static void static_mesh_update(Walrus_StaticMesh *mesh, ecs_world_t *world, ecs_entity_t parent)
{
    if (mesh->mesh->num_weights > 0) {
        walrus_rhi_alloc_transient_buffer(&mesh->weight_buffer, mesh->mesh->num_weights, sizeof(f32));
    }
    else {
        mesh->weight_buffer.handle.id = WR_INVALID_HANDLE;
    }
    Walrus_Model const *model = ecs_get(world, parent, Walrus_Model);
    if (ecs_has(world, parent, Walrus_Animator)) {
        Walrus_Animator const *animator = ecs_get(world, parent, Walrus_Animator);
        walrus_animator_weights(animator, model, mesh->node, (f32 *)mesh->weight_buffer.data);
    }
    else {
        memcpy(mesh->weight_buffer.data, mesh->mesh->weights, sizeof(f32) * mesh->mesh->num_weights);
    }
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

        ecs_entity_t parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);
        mat4         world;
        walrus_transform_compose(&transforms[i], world);

        static_mesh_update(&meshes[i], it->world, parent);
        walrus_deferred_renderer_submit_mesh(renderer, world, &meshes[i]);
    }
}

static void skinned_mesh_update(Walrus_SkinnedMesh *mesh, ecs_world_t *world, ecs_entity_t parent)
{
    Walrus_ModelSkin   *skin  = mesh->skin;
    Walrus_Model const *model = ecs_get(world, parent, Walrus_Model);

    walrus_rhi_alloc_transient_buffer(&mesh->joint_buffer, mesh->skin->num_joints, sizeof(mat4));
    if (mesh->mesh->num_weights > 0) {
        walrus_rhi_alloc_transient_buffer(&mesh->weight_buffer, mesh->mesh->num_weights, sizeof(f32));
    }
    else {
        mesh->weight_buffer.handle.id = WR_INVALID_HANDLE;
    }

    mat4 *m = (mat4 *)mesh->joint_buffer.data;
    if (ecs_has(world, parent, Walrus_Animator)) {
        Walrus_Animator const *animator = ecs_get(world, parent, Walrus_Animator);
        for (u32 i = 0; i < skin->num_joints; ++i) {
            walrus_animator_transform(animator, model, skin->joints[i].node, m[i]);
            glm_mat4_mul(m[i], skin->joints[i].inverse_bind_matrix, m[i]);
        }

        if (mesh->mesh->num_weights > 0) {
            walrus_animator_weights(animator, model, mesh->node, (f32 *)mesh->weight_buffer.data);
        }
    }
    else {
        for (u32 i = 0; i < skin->num_joints; ++i) {
            walrus_transform_compose(&skin->joints[i].node->world_transform, m[i]);
            glm_mat4_mul(m[i], skin->joints[i].inverse_bind_matrix, m[i]);
        }

        if (mesh->mesh->num_weights > 0) {
            memcpy(mesh->weight_buffer.data, mesh->mesh->weights, sizeof(f32) * mesh->mesh->num_weights);
        }
    }
}

static void deferred_submit_skinned_mesh(ecs_iter_t *it)
{
    Walrus_SkinnedMesh      *meshes   = ecs_field(it, Walrus_SkinnedMesh, 1);
    Walrus_Transform        *worlds   = ecs_field(it, Walrus_Transform, 2);
    Walrus_DeferredRenderer *renderer = it->param;

    for (i32 i = 0; i < it->count; ++i) {
        if (meshes[i].culled) {
            continue;
        }

        ecs_entity_t parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);
        mat4         world;
        walrus_transform_compose(&worlds[i], world);

        Walrus_Transform const *p_trans = ecs_get(it->world, parent, Walrus_Transform);

        mat4 p_world;
        walrus_transform_compose(p_trans, p_world);

        skinned_mesh_update(&meshes[i], it->world, parent);
        walrus_deferred_renderer_submit_skinned_mesh(renderer, p_world, &meshes[i]);
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

static void cull_test_skinned_mesh(ecs_iter_t *it)
{
    Walrus_SkinnedMesh *meshes     = ecs_field(it, Walrus_SkinnedMesh, 1);
    Walrus_Transform   *transforms = ecs_field(it, Walrus_Transform, 2);
    Walrus_Camera      *camera     = it->param;

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
        /* char buffer[255]; */
        /* walrus_deferred_renderer_log_stats(&renderers[i], buffer, 255); */
        /* walrus_trace(buffer); */
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

    walrus_trace("render index: %d name: %s", node->index, node->name);
}

static void gbuffer_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    Walrus_DeferredRenderer *renderer = walrus_fg_read_ptr(graph, "DeferredRenderer");
    Walrus_FramebufferHandle gbuffer  = renderer->gbuffer;
    walrus_fg_write(graph, "GBuffer", gbuffer.id);

    ecs_run(ecs, ecs_id(deferred_submit_static_mesh), 0, renderer);
    ecs_run(ecs, ecs_id(deferred_submit_skinned_mesh), 0, renderer);

    walrus_trace("render index: %d name: %s gbuffer: %d", node->index, node->name, gbuffer.id);
}

static void deferred_lighting_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    Walrus_FramebufferHandle gbuffer = {walrus_fg_read(graph, "GBuffer")};

    walrus_trace("render index: %d name: %s gbuffer: %d", node->index, node->name, gbuffer.id);
}

void walrus_render_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_DeferredRenderer);
    ECS_COMPONENT_DEFINE(ecs, Walrus_StaticMesh);
    ECS_COMPONENT_DEFINE(ecs, Walrus_SkinnedMesh);

    ECS_OBSERVER(ecs, on_model_add, EcsOnSet, Walrus_Transform, Walrus_Model);
    ECS_OBSERVER(ecs, on_model_remove, EcsOnRemove, Walrus_Model);

    ECS_SYSTEM_DEFINE(ecs, deferred_submit_static_mesh, 0, Walrus_StaticMesh, Walrus_Transform);
    ECS_SYSTEM_DEFINE(ecs, deferred_submit_skinned_mesh, 0, Walrus_SkinnedMesh, Walrus_Transform);
    ECS_SYSTEM_DEFINE(ecs, deferred_renderer_run, 0, Walrus_DeferredRenderer, Walrus_Camera);
    ECS_SYSTEM_DEFINE(ecs, cull_test_static_mesh, 0, Walrus_StaticMesh, Walrus_Transform);
    ECS_SYSTEM_DEFINE(ecs, cull_test_skinned_mesh, 0, Walrus_SkinnedMesh, Walrus_Transform);

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

    ecs_run(ecs, ecs_id(deferred_renderer_run), 0, NULL);
}
