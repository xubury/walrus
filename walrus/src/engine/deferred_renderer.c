#include <engine/deferred_renderer.h>
#include <engine/shader_library.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/log.h>
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

    Walrus_ProgramHandle shader;
    Walrus_ProgramHandle skin_shader;
    Walrus_ProgramHandle lighting_shader;

    Walrus_UniformHandle u_albedo;
    Walrus_UniformHandle u_albedo_factor;
    Walrus_UniformHandle u_emissive;
    Walrus_UniformHandle u_emissive_factor;
    Walrus_UniformHandle u_normal;
    Walrus_UniformHandle u_normal_scale;
    Walrus_UniformHandle u_alpha_cutoff;
    Walrus_UniformHandle u_has_normal;
    Walrus_UniformHandle u_has_morph;
    Walrus_UniformHandle u_morph_texture;
    Walrus_TextureHandle black_texture;
    Walrus_TextureHandle white_texture;

    Walrus_FramebufferHandle gbuffer;
} RenderData;

static RenderData *s_data = NULL;

static void setup_texture_uniforms(void)
{
    u32 unit = 0;
    walrus_rhi_set_uniform(s_data->u_albedo, 0, sizeof(u32), &unit);
    unit = 1;
    walrus_rhi_set_uniform(s_data->u_emissive, 0, sizeof(u32), &unit);
    unit = 2;
    walrus_rhi_set_uniform(s_data->u_normal, 0, sizeof(u32), &unit);
    unit = 3;
    walrus_rhi_set_uniform(s_data->u_morph_texture, 0, sizeof(u32), &unit);
}

void walrus_deferred_renderer_init_uniforms(void)
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

    s_data->u_albedo          = walrus_rhi_create_uniform("u_albedo", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_albedo_factor   = walrus_rhi_create_uniform("u_albedo_factor", WR_RHI_UNIFORM_VEC4, 1);
    s_data->u_emissive        = walrus_rhi_create_uniform("u_emissive", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_emissive_factor = walrus_rhi_create_uniform("u_emissive_factor", WR_RHI_UNIFORM_VEC3, 1);
    s_data->u_normal          = walrus_rhi_create_uniform("u_normal", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_normal_scale    = walrus_rhi_create_uniform("u_normal_scale", WR_RHI_UNIFORM_FLOAT, 1);
    s_data->u_alpha_cutoff    = walrus_rhi_create_uniform("u_alpha_cutoff", WR_RHI_UNIFORM_FLOAT, 1);
    s_data->u_has_normal      = walrus_rhi_create_uniform("u_has_normal", WR_RHI_UNIFORM_BOOL, 1);
    s_data->u_morph_texture   = walrus_rhi_create_uniform("u_morph_texture", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_has_morph       = walrus_rhi_create_uniform("u_has_morph", WR_RHI_UNIFORM_BOOL, 1);

    Walrus_ShaderHandle vs_mesh         = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_mesh.glsl");
    Walrus_ShaderHandle vs_skinned_mesh = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_skinned_mesh.glsl");
    Walrus_ShaderHandle vs_quad         = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_quad.glsl");
    Walrus_ShaderHandle fs_mesh         = walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_mesh.glsl");
    Walrus_ShaderHandle fs_lighting = walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_deferred_lighting.glsl");

    s_data->shader          = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_mesh, fs_mesh}, 2, true);
    s_data->skin_shader     = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_skinned_mesh, fs_mesh}, 2, true);
    s_data->lighting_shader = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_quad, fs_lighting}, 2, true);

    u32 rgba              = 0;
    s_data->black_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &rgba);
    rgba                  = 0xffffffff;
    s_data->white_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &rgba);

    setup_texture_uniforms();
    // pos normal tangent bitangent albedo emissive depth
    Walrus_Attachment  attachments[7] = {0};
    Walrus_PixelFormat formats[7]     = {WR_RHI_FORMAT_RGB32F, WR_RHI_FORMAT_RGB32F, WR_RHI_FORMAT_RGB32F,
                                         WR_RHI_FORMAT_RGB32F, WR_RHI_FORMAT_RGBA8,  WR_RHI_FORMAT_RGB8,
                                         WR_RHI_FORMAT_DEPTH24};

    u64 flags = WR_RHI_TEXTURE_RT_MSAA_X8;
    for (u32 i = 0; i < walrus_count_of(attachments); ++i) {
        attachments[i].handle = walrus_rhi_create_texture(
            &(Walrus_TextureCreateInfo){
                .ratio = WR_RHI_RATIO_EQUAL, .format = formats[i], .num_mipmaps = 1, .flags = flags},
            NULL);
        attachments[i].access = WR_RHI_ACCESS_WRITE;
    }

    s_data->gbuffer = walrus_rhi_create_framebuffer(attachments, walrus_count_of(attachments));

    walrus_rhi_touch(0);
}

static void setup_primitive(Walrus_MeshPrimitive const *prim)
{
    Walrus_MeshMaterial *material = prim->material;
    if (material) {
        u64 flags = WR_RHI_STATE_DEFAULT;
        if (!material->double_sided) {
            flags |= WR_RHI_STATE_CULL_CW;
        }
        u32 alpha_cutoff = 0.f;
        if (material->alpha_mode == WR_ALPHA_MODE_BLEND) {
            flags |= WR_RHI_STATE_BLEND_ALPHA;
        }
        else if (material->alpha_mode == WR_ALPHA_MODE_MASK) {
            alpha_cutoff = material->alpha_cutoff;
        }
        walrus_rhi_set_state(flags, 0);

        walrus_rhi_set_uniform(s_data->u_alpha_cutoff, 0, sizeof(f32), &alpha_cutoff);

        if (material->albedo) {
            walrus_rhi_set_texture(0, material->albedo->handle);
        }
        else {
            walrus_rhi_set_texture(0, s_data->black_texture);
        }
        walrus_rhi_set_uniform(s_data->u_albedo_factor, 0, sizeof(vec4), material->albedo_factor);

        if (material->emissive) {
            walrus_rhi_set_texture(1, material->emissive->handle);
        }
        else {
            walrus_rhi_set_texture(1, s_data->white_texture);
        }

        bool has_normal = material->normal != NULL;
        walrus_rhi_set_uniform(s_data->u_has_normal, 0, sizeof(bool), &has_normal);
        walrus_rhi_set_uniform(s_data->u_normal_scale, 0, sizeof(f32), &material->normal_scale);
        if (has_normal) {
            walrus_rhi_set_texture(2, material->normal->handle);
        }
        else {
            walrus_rhi_set_texture(2, s_data->black_texture);
        }
        walrus_rhi_set_uniform(s_data->u_emissive_factor, 0, sizeof(vec3), material->emissive_factor);

        bool has_morph = prim->morph_target.id != WR_INVALID_HANDLE;
        walrus_rhi_set_uniform(s_data->u_has_morph, 0, sizeof(bool), &has_morph);
        if (has_morph) {
            walrus_rhi_set_texture(3, prim->morph_target);
        }
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

void walrus_deferred_renderer_set_camera(Walrus_DeferredRenderer *renderer, Walrus_Camera *camera)
{
    renderer->camera  = camera;
    renderer->gbuffer = s_data->gbuffer;

    walrus_rhi_set_view_rect_ratio(0, WR_RHI_RATIO_EQUAL);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0, 1.0, 0);
    walrus_rhi_set_view_transform(0, camera->view, camera->projection);
    walrus_rhi_set_framebuffer(0, renderer->gbuffer);

    walrus_rhi_set_view_rect(1, renderer->x, renderer->y, renderer->width, renderer->height);
    walrus_rhi_set_view_clear(1, WR_RHI_CLEAR_COLOR, 0, 1.0, 0);
    walrus_rhi_set_framebuffer(1, renderer->framebuffer);
}

static void dump_stats(Walrus_DeferredRenderer *renderer, Walrus_Mesh *mesh)
{
    if (renderer->stats.record) {
        renderer->stats.draw_calls += mesh->num_primitives;
        for (u32 i = 0; i < mesh->num_primitives; ++i) {
            Walrus_MeshPrimitive *prim = &mesh->primitives[i];
            renderer->stats.indices += prim->indices.num_indices;
            if (prim->num_streams > 0) {
                renderer->stats.vertices += prim->streams[0].num_vertices;
            }
        }
    }
}

void walrus_deferred_renderer_submit_mesh(Walrus_DeferredRenderer *renderer, mat4 const world, Walrus_Mesh *mesh,
                                          Walrus_TransientBuffer weights)
{
    walrus_rhi_set_transform(world);

    if (weights.handle.id != WR_INVALID_HANDLE) {
        walrus_rhi_set_transient_buffer(0, &weights);
    }

    for (u32 i = 0; i < mesh->num_primitives; ++i) {
        Walrus_MeshPrimitive *prim = &mesh->primitives[i];
        setup_primitive(prim);
        walrus_rhi_submit(0, s_data->shader, 0, WR_RHI_DISCARD_ALL);
    }
    dump_stats(renderer, mesh);
}

void walrus_deferred_renderer_submit_skinned_mesh(Walrus_DeferredRenderer *renderer, mat4 const world,
                                                  Walrus_Mesh *mesh, Walrus_TransientBuffer joints,
                                                  Walrus_TransientBuffer weights)
{
    walrus_rhi_set_transform(world);

    walrus_rhi_set_transient_buffer(0, &joints);
    if (weights.handle.id != WR_INVALID_HANDLE) {
        walrus_rhi_set_transient_buffer(1, &weights);
    }

    for (u32 i = 0; i < mesh->num_primitives; ++i) {
        Walrus_MeshPrimitive *prim = &mesh->primitives[i];
        setup_primitive(prim);
        walrus_rhi_submit(0, s_data->skin_shader, 0, WR_RHI_DISCARD_ALL);
    }
    dump_stats(renderer, mesh);
}

void walrus_deferred_renderer_lighting(void)
{
    walrus_rhi_set_state(WR_RHI_STATE_WRITE_RGB, 0);
    walrus_rhi_set_texture(0, walrus_rhi_get_texture(s_data->gbuffer, 0));
    walrus_rhi_set_vertex_buffer(0, s_data->quad_vertices, s_data->quad_layout, 0, 4);
    walrus_rhi_set_index_buffer(s_data->quad_indices, 0, 6);
    walrus_rhi_submit(1, s_data->lighting_shader, 0, WR_RHI_DISCARD_ALL);
}

void walrus_deferred_renderer_start_record(Walrus_DeferredRenderer *renderer)
{
    renderer->stats.draw_calls = 0;
    renderer->stats.vertices   = 0;
    renderer->stats.indices    = 0;
    renderer->stats.record     = true;
}

void walrus_deferred_renderer_end_record(Walrus_DeferredRenderer *renderer)
{
    renderer->stats.record = false;
}

void walrus_deferred_renderer_log_stats(Walrus_DeferredRenderer *renderer, char *buffer, u32 size)
{
    snprintf(buffer, size, "num of draw calls: %d num of indices: %lld num of vertices: %lld",
             renderer->stats.draw_calls, renderer->stats.indices, renderer->stats.vertices);
}
