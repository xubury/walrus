#include <engine/systems/pipelines/culling_pipeline.h>
#include <engine/systems/render_system.h>
#include <engine/component.h>
#include <engine/engine.h>
#include <engine/camera.h>
#include <core/macro.h>

ECS_SYSTEM_DECLARE(cull_test_static_mesh);
ECS_SYSTEM_DECLARE(cull_test_skinned_mesh);

static void cull_test_skinned_mesh(ecs_iter_t *it)
{
    Walrus_RenderMesh *meshes = ecs_field(it, Walrus_RenderMesh, 1);
    Walrus_Camera     *camera = it->param;

    for (i32 i = 0; i < it->count; ++i) {
        ecs_entity_t parent = ecs_get_target(it->world, it->entities[i], EcsChildOf, 0);

        Walrus_Transform const *p_trans = ecs_get(it->world, parent, Walrus_Transform);

        mat4 p_world;
        walrus_transform_compose(p_trans, p_world);

        meshes[i].culled = false;

        Walrus_SkinResource const *skin = ecs_get(it->world, it->entities[i], Walrus_SkinResource);
        if (!walrus_camera_frustum_cull_test(camera, p_world, skin->min, skin->max)) {
            meshes[i].culled = true;
            continue;
        }
    }
}

static void cull_test_static_mesh(ecs_iter_t *it)
{
    Walrus_RenderMesh *meshes = ecs_field(it, Walrus_RenderMesh, 1);
    Walrus_Transform  *worlds = ecs_field(it, Walrus_Transform, 2);
    Walrus_Camera     *camera = it->param;

    for (i32 i = 0; i < it->count; ++i) {
        mat4 world;
        walrus_transform_compose(&worlds[i], world);

        meshes[i].culled = false;

        if (!walrus_camera_frustum_cull_test(camera, world, meshes[i].mesh->min, meshes[i].mesh->max)) {
            meshes[i].culled = true;
            continue;
        }
    }
}

static void culling_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    walrus_unused(node);

    ecs_world_t   *ecs    = walrus_engine_vars()->ecs;
    Walrus_Camera *camera = walrus_fg_read_ptr(graph, "Camera");

    ecs_run(ecs, ecs_id(cull_test_static_mesh), 0, camera);
    ecs_run(ecs, ecs_id(cull_test_skinned_mesh), 0, camera);
}

Walrus_FramePipeline *walrus_culling_pipeline_add(Walrus_FrameGraph *graph, char const *name)
{
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    ecs_id(cull_test_static_mesh) =
        ecs_system(ecs, {
                            .entity             = ecs_entity(ecs, {0}),
                            .query.filter.terms = {{.id = ecs_id(Walrus_RenderMesh)},
                                                   {.id = ecs_id(Walrus_Transform)},
                                                   {.id = ecs_id(Walrus_SkinResource), .oper = EcsNot}},
                            .callback           = cull_test_static_mesh,
                        });
    ECS_SYSTEM_DEFINE(ecs, cull_test_skinned_mesh, 0, Walrus_RenderMesh, Walrus_SkinResource);

    Walrus_FramePipeline *culling_pipeline = walrus_fg_add_pipeline(graph, name);
    walrus_fg_add_node(culling_pipeline, culling_pass, "Culling");
    return culling_pipeline;
}
