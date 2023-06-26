#include <engine/systems/render_system.h>
#include <engine/systems/animator_system.h>
#include <engine/systems/model_system.h>
#include <engine/systems/transform_system.h>
#include <engine/engine.h>

ECS_COMPONENT_DECLARE(Walrus_DeferredRenderer);
ECS_COMPONENT_DECLARE(Walrus_StaticMesh);
ECS_COMPONENT_DECLARE(Walrus_SkinnedMesh);

ECS_SYSTEM_DECLARE(deferred_render_static_mesh);
ECS_SYSTEM_DECLARE(deferred_render_skinned_mesh);
ECS_SYSTEM_DECLARE(deferred_renderer_run);

static void on_model_add(ecs_iter_t *it)
{
    Walrus_Model *model = ecs_field(it, Walrus_Model, 2);

    for (u32 i = 0; i < model->num_nodes; ++i) {
        Walrus_ModelNode *node = &model->nodes[i];
        ecs_entity_t      mesh = ecs_new_w_pair(it->world, EcsChildOf, it->entities[0]);
        if (node->mesh) {
            Walrus_Transform const *transform = &node->world_transform;
            if (node->skin) {
                ecs_set(it->world, mesh, Walrus_SkinnedMesh, {.mesh = node->mesh, .skin = node->skin, .node = node});
            }
            else {
                ecs_set(it->world, mesh, Walrus_StaticMesh, {.mesh = node->mesh, .node = node});
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

static void deferred_render_static_mesh(ecs_iter_t *it)
{
    Walrus_StaticMesh *meshes     = ecs_field(it, Walrus_StaticMesh, 1);
    Walrus_Transform  *transforms = ecs_field(it, Walrus_Transform, 2);
    Walrus_Camera     *camera     = it->param;

    for (i32 i = 0; i < it->count; ++i) {
        ecs_entity_t parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);
        mat4         world;
        walrus_transform_compose(&transforms[i], world);

        if (!walrus_camera_frustum_cull_test(camera, world, meshes[i].mesh->min, meshes[i].mesh->max)) {
            continue;
        }

        static_mesh_update(&meshes[i], it->world, parent);
        walrus_deferred_renderer_submit_mesh(world, &meshes[i]);
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
        for (u32 j = 0; j < skin->num_joints; ++j) {
            walrus_animator_transform(animator, model, skin->joints[j].node, m[j]);
            glm_mat4_mul(m[j], skin->joints[j].inverse_bind_matrix, m[j]);
        }

        if (mesh->mesh->num_weights > 0) {
            walrus_animator_weights(animator, model, mesh->node, (f32 *)mesh->weight_buffer.data);
        }
    }
    else {
        for (u32 j = 0; j < skin->num_joints; ++j) {
            walrus_transform_compose(&skin->joints[j].node->world_transform, m[j]);
            glm_mat4_mul(m[j], skin->joints[j].inverse_bind_matrix, m[j]);
        }

        if (mesh->mesh->num_weights > 0) {
            memcpy(mesh->weight_buffer.data, mesh->mesh->weights, sizeof(f32) * mesh->mesh->num_weights);
        }
    }
}

static void deferred_render_skinned_mesh(ecs_iter_t *it)
{
    Walrus_SkinnedMesh *meshes = ecs_field(it, Walrus_SkinnedMesh, 1);
    Walrus_Transform   *worlds = ecs_field(it, Walrus_Transform, 2);
    Walrus_Camera      *camera = it->param;

    for (i32 i = 0; i < it->count; ++i) {
        ecs_entity_t parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);
        mat4         world;
        walrus_transform_compose(&worlds[i], world);
        if (!walrus_camera_frustum_cull_test(camera, world, meshes[i].mesh->min, meshes[i].mesh->max)) {
            continue;
        }

        Walrus_Transform const *p_trans = ecs_get(it->world, parent, Walrus_Transform);

        mat4 p_world;
        walrus_transform_compose(p_trans, p_world);

        skinned_mesh_update(&meshes[i], it->world, parent);
        walrus_deferred_renderer_submit_skinned_mesh(p_world, &meshes[i]);
    }
}

static void deferred_renderer_run(ecs_iter_t *it)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    Walrus_DeferredRenderer *renderers = ecs_field(it, Walrus_DeferredRenderer, 1);
    Walrus_Camera           *cameras   = ecs_field(it, Walrus_Camera, 2);

    for (i32 i = 0; i < it->count; ++i) {
        walrus_deferred_renderer_set_camera(&renderers[i], &cameras[i]);
        ecs_run(ecs, ecs_id(deferred_render_static_mesh), 0, &cameras[i]);
        ecs_run(ecs, ecs_id(deferred_render_skinned_mesh), 0, &cameras[i]);
    }
    walrus_rhi_touch(0);
}

void walrus_render_system_init(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;
    ECS_COMPONENT_DEFINE(ecs, Walrus_DeferredRenderer);
    ECS_COMPONENT_DEFINE(ecs, Walrus_StaticMesh);
    ECS_COMPONENT_DEFINE(ecs, Walrus_SkinnedMesh);

    ECS_OBSERVER(ecs, on_model_add, EcsOnSet, Walrus_Transform, Walrus_Model);
    ECS_OBSERVER(ecs, on_model_remove, EcsOnRemove, Walrus_Model);

    ECS_SYSTEM_DEFINE(ecs, deferred_render_static_mesh, 0, Walrus_StaticMesh, Walrus_Transform);
    ECS_SYSTEM_DEFINE(ecs, deferred_render_skinned_mesh, 0, Walrus_SkinnedMesh, Walrus_Transform);
    ECS_SYSTEM_DEFINE(ecs, deferred_renderer_run, 0, Walrus_DeferredRenderer, Walrus_Camera);

    walrus_deferred_renderer_init_uniforms();
}

void walrus_render_system_render(void)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    ecs_run(ecs, ecs_id(deferred_renderer_run), 0, NULL);
}
