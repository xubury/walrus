#include <engine/deferred_renderer.h>
#include <engine/shader_library.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/math.h>
#include <rhi/rhi.h>

#include <cglm/cglm.h>
#include <string.h>

static struct {
    vec2 pos;
    vec2 uv;
} quad_vertices[] = {{{-1.0, 1.0}, {0, 1}}, {{1.0, 1.0}, {1, 1}}, {{1.0, -1.0}, {1, 0}}, {{-1.0, -1.0}, {0, 0}}};

static u16 quad_indices[] = {0, 1, 2, 2, 3, 0};

typedef struct {
    Walrus_BufferHandle quad_vertices;
    Walrus_BufferHandle quad_indices;
    Walrus_LayoutHandle quad_layout;

    Walrus_ProgramHandle gbuffer_shader;
    Walrus_ProgramHandle gbuffer_skin_shader;
    Walrus_ProgramHandle deferred_shader;
    Walrus_ProgramHandle forward_shader;
    Walrus_ProgramHandle forward_skin_shader;

    Walrus_ProgramHandle copy_shader;
    Walrus_ProgramHandle hdr_shader;

    Walrus_UniformHandle u_has_morph;
    Walrus_UniformHandle u_morph_texture;

    Walrus_UniformHandle u_gpos;
    Walrus_UniformHandle u_gnormal;
    Walrus_UniformHandle u_gtangnent;
    Walrus_UniformHandle u_gbitangent;
    Walrus_UniformHandle u_galbedo;
    Walrus_UniformHandle u_gemissive;

    Walrus_UniformHandle u_color_buffer;
    Walrus_UniformHandle u_depth_buffer;

    Walrus_TextureHandle     depth_buffer;
    Walrus_FramebufferHandle gbuffer;
    Walrus_FramebufferHandle hdr_buffer;
    Walrus_FramebufferHandle back_buffer;
} RenderData;

static RenderData *s_data = NULL;

typedef enum {
    G_POS,
    G_NORMAL,
    G_TANGENT,
    G_BITANGET,
    G_ALBEDO,
    G_EMISSIVE
} GBufferTexture;

void walrus_deferred_renderer_init(u8 msaa)
{
    s_data = walrus_new(RenderData, 1);

    Walrus_VertexLayout layout;
    walrus_vertex_layout_begin(&layout);
    walrus_vertex_layout_add(&layout, 0, 2, WR_RHI_COMPONENT_FLOAT, false);  // Pos
    walrus_vertex_layout_add(&layout, 1, 2, WR_RHI_COMPONENT_FLOAT, false);  // TexCoord
    walrus_vertex_layout_end(&layout);
    s_data->quad_layout = walrus_rhi_create_vertex_layout(&layout);

    s_data->quad_vertices = walrus_rhi_create_buffer(quad_vertices, sizeof(quad_vertices), 0);
    s_data->quad_indices  = walrus_rhi_create_buffer(quad_indices, sizeof(quad_indices), WR_RHI_BUFFER_INDEX);

    walrus_rhi_create_uniform("u_albedo", WR_RHI_UNIFORM_SAMPLER, 1);
    walrus_rhi_create_uniform("u_albedo_factor", WR_RHI_UNIFORM_VEC4, 1);
    walrus_rhi_create_uniform("u_emissive", WR_RHI_UNIFORM_SAMPLER, 1);
    walrus_rhi_create_uniform("u_emissive_factor", WR_RHI_UNIFORM_VEC3, 1);
    walrus_rhi_create_uniform("u_normal", WR_RHI_UNIFORM_SAMPLER, 1);
    walrus_rhi_create_uniform("u_normal_scale", WR_RHI_UNIFORM_FLOAT, 1);
    walrus_rhi_create_uniform("u_alpha_cutoff", WR_RHI_UNIFORM_FLOAT, 1);

    s_data->u_morph_texture = walrus_rhi_create_uniform("u_morph_texture", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_has_morph     = walrus_rhi_create_uniform("u_has_morph", WR_RHI_UNIFORM_BOOL, 1);

    s_data->u_gpos       = walrus_rhi_create_uniform("u_gpos", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gnormal    = walrus_rhi_create_uniform("u_gnormal", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gtangnent  = walrus_rhi_create_uniform("u_gtangent", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gbitangent = walrus_rhi_create_uniform("u_gbitangent", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_galbedo    = walrus_rhi_create_uniform("u_galbedo", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gemissive  = walrus_rhi_create_uniform("u_gemissive", WR_RHI_UNIFORM_SAMPLER, 1);

    s_data->u_color_buffer = walrus_rhi_create_uniform("u_color_buffer", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_depth_buffer = walrus_rhi_create_uniform("u_depth_buffer", WR_RHI_UNIFORM_SAMPLER, 1);

    s_data->gbuffer_shader      = walrus_shader_library_load("gbuffer.shader");
    s_data->gbuffer_skin_shader = walrus_shader_library_load("gbuffer_skin.shader");
    s_data->deferred_shader     = walrus_shader_library_load("deferred_lighting.shader");
    s_data->forward_shader      = walrus_shader_library_load("forward_lighting.shader");
    s_data->forward_skin_shader = walrus_shader_library_load("forward_lighting_skin.shader");
    s_data->copy_shader         = walrus_shader_library_load("copy.shader");
    s_data->hdr_shader          = walrus_shader_library_load("hdr.shader");

    u64 flags            = (u64)(walrus_u32cnttz(msaa) + 1) << WR_RHI_TEXTURE_RT_MSAA_SHIFT;
    s_data->depth_buffer = walrus_rhi_create_texture(
        &(Walrus_TextureCreateInfo){
            .ratio = WR_RHI_RATIO_EQUAL, .format = WR_RHI_FORMAT_DEPTH24, .num_mipmaps = 1, .flags = flags},
        NULL);
    {
        Walrus_Attachment attachments[2] = {0};

        attachments[0].handle = walrus_rhi_create_texture(
            &(Walrus_TextureCreateInfo){
                .ratio = WR_RHI_RATIO_EQUAL, .format = WR_RHI_FORMAT_RGBA8, .num_mipmaps = 1, .flags = flags},
            NULL);
        attachments[0].access = WR_RHI_ACCESS_WRITE;
        attachments[1].handle = s_data->depth_buffer;
        attachments[1].access = WR_RHI_ACCESS_WRITE;

        s_data->back_buffer = walrus_rhi_create_framebuffer(attachments, walrus_count_of(attachments));
    }
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

        attachments[walrus_count_of(attachments) - 1].handle = s_data->depth_buffer;

        for (u32 i = 0; i < walrus_count_of(formats); ++i) {
            attachments[i].handle = walrus_rhi_create_texture(
                &(Walrus_TextureCreateInfo){
                    .ratio = WR_RHI_RATIO_EQUAL, .format = formats[i], .num_mipmaps = 1, .flags = flags},
                NULL);
            attachments[i].access = WR_RHI_ACCESS_WRITE;
        }

        s_data->gbuffer = walrus_rhi_create_framebuffer(attachments, walrus_count_of(attachments));
    }
    {
        Walrus_Attachment attachment = {0};

        attachment.handle = walrus_rhi_create_texture(
            &(Walrus_TextureCreateInfo){
                .ratio = WR_RHI_RATIO_EQUAL, .format = WR_RHI_FORMAT_RGB8, .num_mipmaps = 1, .flags = 0},
            NULL);
        attachment.access  = WR_RHI_ACCESS_WRITE;
        s_data->hdr_buffer = walrus_rhi_create_framebuffer(&attachment, 1);
    }

    walrus_rhi_touch(0);
}

static void setup_primitive(Walrus_MeshPrimitive const *prim)
{
    bool has_morph = prim->morph_target.id != WR_INVALID_HANDLE;
    walrus_rhi_set_uniform(s_data->u_has_morph, 0, sizeof(bool), &has_morph);
    if (has_morph) {
        u32 unit = walrus_rhi_get_caps()->max_texture_unit - 1;
        walrus_rhi_set_uniform(s_data->u_morph_texture, 0, sizeof(u32), &unit);
        walrus_rhi_set_texture(unit, prim->morph_target);
    }
    if (prim->indices.buffer.id != WR_INVALID_HANDLE) {
        if (prim->indices.index32) {
            walrus_rhi_set_index32_buffer(prim->indices.buffer, prim->indices.offset, prim->indices.num_indices);
        }
        else {
            walrus_rhi_set_index_buffer(prim->indices.buffer, prim->indices.offset, prim->indices.num_indices);
        }
    }
    for (u32 j = 0; j < prim->num_streams; ++j) {
        Walrus_PrimitiveStream const *stream = &prim->streams[j];
        walrus_rhi_set_vertex_buffer(j, stream->buffer, stream->layout_handle, stream->offset, stream->num_vertices);
    }
}

void walrus_deferred_renderer_set_camera(Walrus_Renderer const *renderer, Walrus_Camera const *camera)
{
    walrus_rhi_set_view_rect_ratio(0, WR_RHI_RATIO_EQUAL);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_DEPTH | WR_RHI_CLEAR_COLOR, 0, 1.0, 0);
    walrus_rhi_set_view_transform(0, camera->view, camera->projection);
    walrus_rhi_set_framebuffer(0, s_data->gbuffer);

    walrus_rhi_set_view_rect(1, renderer->x, renderer->y, renderer->width, renderer->height);
    walrus_rhi_set_view_clear(1, WR_RHI_CLEAR_COLOR, 0, 1.0, 0);
    walrus_rhi_set_view_transform(1, camera->view, camera->projection);
    walrus_rhi_set_framebuffer(1, s_data->back_buffer);

    walrus_rhi_set_view_rect_ratio(2, WR_RHI_RATIO_EQUAL);
    walrus_rhi_set_view_clear(2, WR_RHI_CLEAR_NONE, 0, 1.0, 0);
    walrus_rhi_set_framebuffer(2, s_data->hdr_buffer);

    walrus_rhi_set_view_rect_ratio(3, WR_RHI_RATIO_EQUAL);
    walrus_rhi_set_view_clear(3, WR_RHI_CLEAR_NONE, 0, 1.0, 0);
    walrus_rhi_set_framebuffer(3, (Walrus_FramebufferHandle){WR_INVALID_HANDLE});
}

void walrus_deferred_renderer_submit_mesh(mat4 const world, Walrus_Mesh *mesh, Walrus_TransientBuffer const *weights)
{
    walrus_rhi_set_transform(world);

    if (weights) {
        walrus_rhi_set_transient_buffer(0, weights);
    }

    for (u32 i = 0; i < mesh->num_primitives; ++i) {
        Walrus_MeshPrimitive *prim = &mesh->primitives[i];
        setup_primitive(prim);
        walrus_rhi_submit(0, s_data->gbuffer_shader, 0, WR_RHI_DISCARD_ALL);
    }
}

void walrus_deferred_renderer_submit_skinned_mesh(mat4 const world, Walrus_Mesh *mesh,
                                                  Walrus_TransientBuffer const *joints,
                                                  Walrus_TransientBuffer const *weights)
{
    walrus_rhi_set_transform(world);

    walrus_rhi_set_transient_buffer(0, joints);
    if (weights) {
        walrus_rhi_set_transient_buffer(1, weights);
    }

    for (u32 i = 0; i < mesh->num_primitives; ++i) {
        Walrus_MeshPrimitive *prim = &mesh->primitives[i];

        setup_primitive(prim);
        walrus_rhi_submit(0, s_data->gbuffer_skin_shader, 0, WR_RHI_DISCARD_ALL);
    }
}

void walrus_deferred_renderer_submit_mesh_ablend(mat4 const world, Walrus_Mesh *mesh,
                                                 Walrus_TransientBuffer const *weights)
{
    walrus_rhi_set_transform(world);

    if (weights) {
        walrus_rhi_set_transient_buffer(0, weights);
    }

    for (u32 i = 0; i < mesh->num_primitives; ++i) {
        Walrus_MeshPrimitive *prim = &mesh->primitives[i];

        setup_primitive(prim);
        walrus_rhi_submit(1, s_data->forward_shader, 0, WR_RHI_DISCARD_ALL);
    }
}

void walrus_deferred_renderer_submit_skinned_mesh_ablend(mat4 const world, Walrus_Mesh *mesh,
                                                         Walrus_TransientBuffer const *joints,
                                                         Walrus_TransientBuffer const *weights)
{
    walrus_rhi_set_transform(world);

    walrus_rhi_set_transient_buffer(0, joints);
    if (weights) {
        walrus_rhi_set_transient_buffer(1, weights);
    }

    for (u32 i = 0; i < mesh->num_primitives; ++i) {
        Walrus_MeshPrimitive *prim = &mesh->primitives[i];
        setup_primitive(prim);
        walrus_rhi_submit(1, s_data->forward_skin_shader, 0, WR_RHI_DISCARD_ALL);
    }
}

void walrus_deferred_renderer_lighting(void)
{
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
}

void walrus_renderer_submit_hdr(void)
{
    walrus_rhi_set_uniform(s_data->u_color_buffer, 0, sizeof(u32), &(u32){0});
    walrus_rhi_set_texture(0, walrus_rhi_get_texture(s_data->back_buffer, 0));

    walrus_rhi_set_state(WR_RHI_STATE_WRITE_RGB | WR_RHI_STATE_WRITE_A, 0);
    walrus_renderer_submit_quad(2, s_data->hdr_shader);
}

void walrus_renderer_submit_backbuffer(void)
{
    walrus_rhi_set_uniform(s_data->u_color_buffer, 0, sizeof(u32), &(u32){0});
    walrus_rhi_set_texture(0, walrus_rhi_get_texture(s_data->hdr_buffer, 0));

    walrus_rhi_set_uniform(s_data->u_depth_buffer, 0, sizeof(u32), &(u32){1});
    walrus_rhi_set_texture(1, s_data->depth_buffer);

    walrus_rhi_set_state(WR_RHI_STATE_WRITE_RGB | WR_RHI_STATE_WRITE_A | WR_RHI_STATE_WRITE_Z, 0);
    walrus_renderer_submit_quad(3, s_data->copy_shader);
}

void walrus_renderer_submit_quad(u16 view_id, Walrus_ProgramHandle shader)
{
    walrus_rhi_set_vertex_buffer(0, s_data->quad_vertices, s_data->quad_layout, 0, 4);
    walrus_rhi_set_index_buffer(s_data->quad_indices, 0, 6);
    walrus_rhi_submit(view_id, shader, 0, WR_RHI_DISCARD_ALL);
}
