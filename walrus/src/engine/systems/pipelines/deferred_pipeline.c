#include <engine/systems/pipelines/deferred_pipeline.h>
#include <engine/systems/render_system.h>
#include <engine/shader_library.h>
#include <engine/component.h>
#include <engine/engine.h>
#include <core/macro.h>
#include <core/memory.h>
#include <core/math.h>

ECS_SYSTEM_DECLARE(deferred_submit_static_mesh);
ECS_SYSTEM_DECLARE(deferred_submit_skinned_mesh);
ECS_SYSTEM_DECLARE(forward_submit_static_mesh);
ECS_SYSTEM_DECLARE(forward_submit_skinned_mesh);

typedef enum {
    G_POS,
    G_NORMAL,
    G_TANGENT,
    G_BITANGET,
    G_ALBEDO,
    G_EMISSIVE
} GBufferTexture;

typedef struct {
    Walrus_ProgramHandle gbuffer_shader;
    Walrus_ProgramHandle gbuffer_skin_shader;
    Walrus_ProgramHandle forward_shader;
    Walrus_ProgramHandle forward_skin_shader;

    Walrus_ProgramHandle deferred_shader;

    Walrus_UniformHandle u_gpos;
    Walrus_UniformHandle u_gnormal;
    Walrus_UniformHandle u_gtangnent;
    Walrus_UniformHandle u_gbitangent;
    Walrus_UniformHandle u_galbedo;
    Walrus_UniformHandle u_gemissive;

    Walrus_FramebufferHandle gbuffer;
} DeferredRenderData;

DeferredRenderData *s_data = NULL;

static void gbuffer_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    walrus_unused(node);
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    u16           *view_id = walrus_fg_read_ptr(graph, "ViewSlot");
    Walrus_Camera *camera  = walrus_fg_read_ptr(graph, "Camera");

    walrus_rhi_set_view_rect_ratio(*view_id, WR_RHI_RATIO_EQUAL);
    walrus_rhi_set_view_clear(*view_id, WR_RHI_CLEAR_DEPTH | WR_RHI_CLEAR_COLOR, 0, 1.0, 0);
    walrus_rhi_set_view_transform(*view_id, camera->view, camera->projection);
    walrus_rhi_set_framebuffer(*view_id, s_data->gbuffer);

    ecs_run(ecs, ecs_id(deferred_submit_static_mesh), 0, view_id);
    ecs_run(ecs, ecs_id(deferred_submit_skinned_mesh), 0, view_id);

    ++(*view_id);
}

static void lighting_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    walrus_unused(node);
    ecs_world_t *ecs = walrus_engine_vars()->ecs;

    u16                     *view_id = walrus_fg_read_ptr(graph, "ViewSlot");
    Walrus_Camera           *camera  = walrus_fg_read_ptr(graph, "Camera");
    Walrus_FramebufferHandle backrt  = {walrus_fg_read(graph, "BackRT")};

    walrus_rhi_set_view_rect_ratio(*view_id, WR_RHI_RATIO_EQUAL);
    walrus_rhi_set_view_clear(*view_id, WR_RHI_CLEAR_COLOR, 0, 1.0, 0);
    walrus_rhi_set_view_transform(*view_id, camera->view, camera->projection);
    walrus_rhi_set_framebuffer(*view_id, backrt);

    walrus_rhi_set_uniform(s_data->u_gpos, 0, sizeof(u32), &(u32){G_POS});
    walrus_rhi_set_uniform(s_data->u_gnormal, 0, sizeof(u32), &(u32){G_NORMAL});
    walrus_rhi_set_uniform(s_data->u_galbedo, 0, sizeof(u32), &(u32){G_ALBEDO});
    walrus_rhi_set_uniform(s_data->u_gemissive, 0, sizeof(u32), &(u32){G_EMISSIVE});

    walrus_rhi_set_texture(G_POS, walrus_rhi_get_texture(s_data->gbuffer, G_POS));
    walrus_rhi_set_texture(G_NORMAL, walrus_rhi_get_texture(s_data->gbuffer, G_NORMAL));
    walrus_rhi_set_texture(G_ALBEDO, walrus_rhi_get_texture(s_data->gbuffer, G_ALBEDO));
    walrus_rhi_set_texture(G_EMISSIVE, walrus_rhi_get_texture(s_data->gbuffer, G_EMISSIVE));

    walrus_rhi_set_state(WR_RHI_STATE_WRITE_RGB, 0);
    walrus_renderer_submit_quad(1, s_data->deferred_shader);

    ecs_run(ecs, ecs_id(forward_submit_static_mesh), 0, view_id);
    ecs_run(ecs, ecs_id(forward_submit_skinned_mesh), 0, view_id);

    ++(*view_id);
}

static void deferred_submit_static_mesh(ecs_iter_t *it)
{
    Walrus_RenderMesh *meshes     = ecs_field(it, Walrus_RenderMesh, 1);
    Walrus_Material   *materials  = ecs_field(it, Walrus_Material, 2);
    Walrus_Transform  *transforms = ecs_field(it, Walrus_Transform, 3);
    u16                view_id    = *(u16 *)it->param;

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
            walrus_rhi_set_transient_buffer(0, weights);
        }
        walrus_material_submit(&materials[i]);
        walrus_renderer_submit_mesh(view_id, s_data->gbuffer_shader, world, meshes[i].mesh);
    }
}

static void deferred_submit_skinned_mesh(ecs_iter_t *it)
{
    Walrus_RenderMesh   *meshes    = ecs_field(it, Walrus_RenderMesh, 1);
    Walrus_Material     *materials = ecs_field(it, Walrus_Material, 2);
    Walrus_SkinResource *skins     = ecs_field(it, Walrus_SkinResource, 3);
    u16                  view_id   = *(u16 *)it->param;

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

        if (ecs_has(it->world, it->entities[i], Walrus_WeightResource)) {
            walrus_rhi_set_transient_buffer(0,
                                            &ecs_get(it->world, it->entities[i], Walrus_WeightResource)->weight_buffer);
        }
        walrus_rhi_set_transient_buffer(1, &skins[i].joint_buffer);
        walrus_material_submit(&materials[i]);
        walrus_renderer_submit_mesh(view_id, s_data->gbuffer_skin_shader, p_world, meshes[i].mesh);
    }
}

static void forward_submit_static_mesh(ecs_iter_t *it)
{
    Walrus_RenderMesh *meshes     = ecs_field(it, Walrus_RenderMesh, 1);
    Walrus_Material   *materials  = ecs_field(it, Walrus_Material, 2);
    Walrus_Transform  *transforms = ecs_field(it, Walrus_Transform, 3);
    u16                view_id    = *(u16 *)it->param;

    for (i32 i = 0; i < it->count; ++i) {
        if (meshes[i].culled) {
            continue;
        }
        if (materials[i].alpha_mode != WR_ALPHA_MODE_BLEND) {
            continue;
        }

        mat4 world;
        walrus_transform_compose(&transforms[i], world);

        if (ecs_has(it->world, it->entities[i], Walrus_WeightResource)) {
            walrus_rhi_set_transient_buffer(0,
                                            &ecs_get(it->world, it->entities[i], Walrus_WeightResource)->weight_buffer);
        }
        walrus_material_submit(&materials[i]);
        walrus_renderer_submit_mesh(view_id, s_data->forward_shader, world, meshes[i].mesh);
    }
}

static void render_data_free(void *userdata)
{
    walrus_unused(userdata);
    walrus_rhi_destroy_uniform(s_data->u_gpos);
    walrus_rhi_destroy_uniform(s_data->u_gnormal);
    walrus_rhi_destroy_uniform(s_data->u_gtangnent);
    walrus_rhi_destroy_uniform(s_data->u_gbitangent);
    walrus_rhi_destroy_uniform(s_data->u_galbedo);
    walrus_rhi_destroy_uniform(s_data->u_gemissive);

    walrus_rhi_destroy_framebuffer(s_data->gbuffer);

    walrus_free(s_data);
}

static void render_data_create(Walrus_FrameGraph *graph)
{
    s_data = walrus_new(DeferredRenderData, 1);

    s_data->u_gpos       = walrus_rhi_create_uniform("u_gpos", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gnormal    = walrus_rhi_create_uniform("u_gnormal", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gtangnent  = walrus_rhi_create_uniform("u_gtangent", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gbitangent = walrus_rhi_create_uniform("u_gbitangent", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_galbedo    = walrus_rhi_create_uniform("u_galbedo", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gemissive  = walrus_rhi_create_uniform("u_gemissive", WR_RHI_UNIFORM_SAMPLER, 1);

    s_data->gbuffer_shader      = walrus_shader_library_load("gbuffer.shader");
    s_data->gbuffer_skin_shader = walrus_shader_library_load("gbuffer_skin.shader");
    s_data->forward_shader      = walrus_shader_library_load("forward_lighting.shader");
    s_data->forward_skin_shader = walrus_shader_library_load("forward_lighting_skin.shader");
    s_data->deferred_shader     = walrus_shader_library_load("deferred_lighting.shader");

    u64 flags = (u64)(walrus_u32cnttz(walrus_rhi_get_mssa()) + 1) << WR_RHI_TEXTURE_RT_MSAA_SHIFT;

    {
        Walrus_Attachment  attachments[7] = {0};
        Walrus_PixelFormat formats[6]     = {
            WR_RHI_FORMAT_RGB32F,  // pos
            WR_RHI_FORMAT_RGB32F,  // normal
            WR_RHI_FORMAT_RGB32F,  // tangent
            WR_RHI_FORMAT_RGB32F,  // bitangent
            WR_RHI_FORMAT_RGBA8,   // albedo
            WR_RHI_FORMAT_RGB8     // emissive
        };

        attachments[walrus_count_of(attachments) - 1].handle =
            (Walrus_TextureHandle){walrus_fg_read(graph, "DepthBuffer")};

        for (u32 i = 0; i < walrus_count_of(formats); ++i) {
            attachments[i].handle = walrus_rhi_create_texture(
                &(Walrus_TextureCreateInfo){
                    .ratio = WR_RHI_RATIO_EQUAL, .format = formats[i], .num_mipmaps = 1, .flags = flags},
                NULL);
            attachments[i].access = WR_RHI_ACCESS_WRITE;
        }

        s_data->gbuffer = walrus_rhi_create_framebuffer(attachments, walrus_count_of(attachments));
    }
}

static void forward_submit_skinned_mesh(ecs_iter_t *it)
{
    Walrus_RenderMesh   *meshes    = ecs_field(it, Walrus_RenderMesh, 1);
    Walrus_Material     *materials = ecs_field(it, Walrus_Material, 2);
    Walrus_SkinResource *skins     = ecs_field(it, Walrus_SkinResource, 3);
    u16                  view_id   = *(u16 *)it->param;

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

        if (ecs_has(it->world, it->entities[i], Walrus_WeightResource)) {
            walrus_rhi_set_transient_buffer(0,
                                            &ecs_get(it->world, it->entities[i], Walrus_WeightResource)->weight_buffer);
        }
        walrus_material_submit(&materials[i]);
        walrus_rhi_set_transient_buffer(1, &skins[i].joint_buffer);
        walrus_renderer_submit_mesh(view_id, s_data->forward_skin_shader, p_world, meshes[i].mesh);
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

    render_data_create(graph);

    Walrus_FramePipeline *deferred_pipeline = walrus_fg_add_pipeline_full(graph, name, render_data_free, NULL);
    walrus_fg_add_node(deferred_pipeline, gbuffer_pass, "GBuffer");
    walrus_fg_add_node(deferred_pipeline, lighting_pass, "Lighting");

    return deferred_pipeline;
}
