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

    Walrus_ProgramHandle gbuffer_shader;
    Walrus_ProgramHandle gbuffer_skin_shader;
    Walrus_ProgramHandle deferred_shader;
    Walrus_ProgramHandle forward_shader;
    Walrus_ProgramHandle forward_skin_shader;

    Walrus_ProgramHandle depth_shader;
    Walrus_ProgramHandle depth_skin_shader;

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

    Walrus_UniformHandle u_gpos;
    Walrus_UniformHandle u_gnormal;
    Walrus_UniformHandle u_gtangnent;
    Walrus_UniformHandle u_gbitangent;
    Walrus_UniformHandle u_galbedo;
    Walrus_UniformHandle u_gemissive;
    Walrus_UniformHandle u_gdepth;

    Walrus_FramebufferHandle gbuffer;
} RenderData;

static RenderData *s_data = NULL;

static void setup_texture_uniforms(void)
{
    walrus_rhi_set_uniform(s_data->u_albedo, 0, sizeof(u32), &(u32){0});
    walrus_rhi_set_uniform(s_data->u_emissive, 0, sizeof(u32), &(u32){1});
    walrus_rhi_set_uniform(s_data->u_normal, 0, sizeof(u32), &(u32){2});
    walrus_rhi_set_uniform(s_data->u_morph_texture, 0, sizeof(u32), &(u32){3});
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

    s_data->u_gpos       = walrus_rhi_create_uniform("u_gpos", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gnormal    = walrus_rhi_create_uniform("u_gnormal", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gtangnent  = walrus_rhi_create_uniform("u_gtangent", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gbitangent = walrus_rhi_create_uniform("u_gbitangent", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_galbedo    = walrus_rhi_create_uniform("u_galbedo", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gemissive  = walrus_rhi_create_uniform("u_gemissive", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_gdepth     = walrus_rhi_create_uniform("u_gdepth", WR_RHI_UNIFORM_SAMPLER, 1);

    Walrus_ShaderHandle vs_mesh         = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_mesh.glsl");
    Walrus_ShaderHandle vs_skinned_mesh = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_skinned_mesh.glsl");
    Walrus_ShaderHandle vs_quad         = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_quad.glsl");
    Walrus_ShaderHandle fs_gbuffer      = walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_gbuffer.glsl");
    Walrus_ShaderHandle fs_deferred = walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_deferred_lighting.glsl");
    Walrus_ShaderHandle fs_forwrad  = walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_forward_lighting.glsl");
    Walrus_ShaderHandle fs_empty    = walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_empty.glsl");

    s_data->gbuffer_shader = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_mesh, fs_gbuffer}, 2, true);
    s_data->gbuffer_skin_shader =
        walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_skinned_mesh, fs_gbuffer}, 2, true);
    s_data->deferred_shader = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_quad, fs_deferred}, 2, true);
    s_data->forward_shader  = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_mesh, fs_forwrad}, 2, true);
    s_data->forward_skin_shader =
        walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_skinned_mesh, fs_forwrad}, 2, true);
    s_data->depth_shader      = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_mesh, fs_empty}, 2, true);
    s_data->depth_skin_shader = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_skinned_mesh, fs_empty}, 2, true);

    u32 rgba              = 0;
    s_data->black_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &rgba);
    rgba                  = 0xffffffff;
    s_data->white_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &rgba);

    setup_texture_uniforms();
    Walrus_Attachment  attachments[7] = {0};
    Walrus_PixelFormat formats[7]     = {WR_RHI_FORMAT_RGB32F,  // pos
                                         WR_RHI_FORMAT_RGB32F,  // normal
                                         WR_RHI_FORMAT_RGB32F,  // tangent
                                         WR_RHI_FORMAT_RGB32F,  // bitangent
                                         WR_RHI_FORMAT_RGBA8,   // albedo
                                         WR_RHI_FORMAT_RGB8,    // emissive
                                         WR_RHI_FORMAT_DEPTH24};

    u64 extra_flags[7] = {0, 0, 0, 0, 0, 0, 0};

    u64 flags = WR_RHI_TEXTURE_RT_MSAA_X8;
    for (u32 i = 0; i < walrus_count_of(attachments); ++i) {
        attachments[i].handle = walrus_rhi_create_texture(
            &(Walrus_TextureCreateInfo){
                .ratio = WR_RHI_RATIO_EQUAL, .format = formats[i], .num_mipmaps = 1, .flags = flags | extra_flags[i]},
            NULL);
        attachments[i].access = WR_RHI_ACCESS_WRITE;
    }

    s_data->gbuffer = walrus_rhi_create_framebuffer(attachments, walrus_count_of(attachments));

    walrus_rhi_touch(0);
}

static bool setup_primitive(Walrus_MeshPrimitive const *prim, u64 flags, bool opaque)
{
    Walrus_MeshMaterial *material = prim->material;
    if (material) {
        if (!material->double_sided) {
            flags |= WR_RHI_STATE_CULL_CW;
        }
        u32 alpha_cutoff = 0.f;
        if (material->alpha_mode == WR_ALPHA_MODE_BLEND) {
            flags |= WR_RHI_STATE_BLEND_ALPHA;
            if (opaque) {
                return false;
            }
        }
        else if (!opaque) {
            return false;
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
    }
    bool has_morph = prim->morph_target.id != WR_INVALID_HANDLE;
    walrus_rhi_set_uniform(s_data->u_has_morph, 0, sizeof(bool), &has_morph);
    if (has_morph) {
        walrus_rhi_set_texture(3, prim->morph_target);
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

    return true;
}

void walrus_deferred_renderer_set_camera(Walrus_DeferredRenderer *renderer, Walrus_Camera *camera)
{
    renderer->camera = camera;

    walrus_rhi_set_view_rect(0, renderer->x, renderer->y, renderer->width, renderer->height);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_DEPTH, 0, 1.0, 0);
    walrus_rhi_set_view_transform(0, camera->view, camera->projection);
    walrus_rhi_set_framebuffer(0, renderer->framebuffer);

    walrus_rhi_set_view_rect_ratio(1, WR_RHI_RATIO_EQUAL);
    walrus_rhi_set_view_clear(1, WR_RHI_CLEAR_DEPTH | WR_RHI_CLEAR_COLOR, 0, 1.0, 0);
    walrus_rhi_set_view_transform(1, camera->view, camera->projection);
    walrus_rhi_set_framebuffer(1, s_data->gbuffer);

    walrus_rhi_set_view_rect(2, renderer->x, renderer->y, renderer->width, renderer->height);
    walrus_rhi_set_view_clear(2, WR_RHI_CLEAR_COLOR, 0, 1.0, 0);
    walrus_rhi_set_view_transform(2, camera->view, camera->projection);
    walrus_rhi_set_framebuffer(2, renderer->framebuffer);
}

static void dump_stats(Walrus_DeferredRenderer *renderer, Walrus_MeshPrimitive *prim)
{
    if (renderer->stats.record) {
        renderer->stats.draw_calls += 1;
        renderer->stats.indices += prim->indices.num_indices;
        if (prim->num_streams > 0) {
            renderer->stats.vertices += prim->streams[0].num_vertices;
        }
    }
}

void walrus_renderer_submit_mesh(Walrus_DeferredRenderer *renderer, mat4 const world, Walrus_Mesh *mesh,
                                 Walrus_TransientBuffer weights)
{
    walrus_rhi_set_transform(world);

    if (weights.handle.id != WR_INVALID_HANDLE) {
        walrus_rhi_set_transient_buffer(0, &weights);
    }
    for (u32 i = 0; i < mesh->num_primitives; ++i) {
        Walrus_MeshPrimitive *prim = &mesh->primitives[i];

        u64 flags = WR_RHI_STATE_WRITE_Z | WR_RHI_STATE_DEPTH_TEST_LESS;
        if (prim->material) {
            if (!prim->material->double_sided) {
                flags |= WR_RHI_STATE_CULL_CW;
            }
            if (prim->material->alpha_mode != WR_ALPHA_MODE_OPAQUE) {
                continue;
            }
        }
        walrus_rhi_set_state(flags, 0);

        bool has_morph = prim->morph_target.id != WR_INVALID_HANDLE;
        walrus_rhi_set_uniform(s_data->u_has_morph, 0, sizeof(bool), &has_morph);
        if (has_morph) {
            walrus_rhi_set_texture(3, prim->morph_target);
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
            walrus_rhi_set_vertex_buffer(j, stream->buffer, stream->layout_handle, stream->offset,
                                         stream->num_vertices);
        }
        walrus_rhi_submit(0, s_data->depth_shader, 0, WR_RHI_DISCARD_ALL);
        dump_stats(renderer, prim);
    }
}

void walrus_renderer_submit_skinned_mesh(Walrus_DeferredRenderer *renderer, mat4 const world, Walrus_Mesh *mesh,
                                         Walrus_TransientBuffer joints, Walrus_TransientBuffer weights)
{
    walrus_rhi_set_transform(world);

    walrus_rhi_set_transient_buffer(0, &joints);
    if (weights.handle.id != WR_INVALID_HANDLE) {
        walrus_rhi_set_transient_buffer(1, &weights);
    }
    for (u32 i = 0; i < mesh->num_primitives; ++i) {
        Walrus_MeshPrimitive *prim = &mesh->primitives[i];

        u64 flags = WR_RHI_STATE_WRITE_Z | WR_RHI_STATE_DEPTH_TEST_LESS;
        if (prim->material) {
            if (prim->material->alpha_mode != WR_ALPHA_MODE_OPAQUE) {
                continue;
            }
            if (!prim->material->double_sided) {
                flags |= WR_RHI_STATE_CULL_CW;
            }
        }
        walrus_rhi_set_state(flags, 0);

        bool has_morph = prim->morph_target.id != WR_INVALID_HANDLE;
        walrus_rhi_set_uniform(s_data->u_has_morph, 0, sizeof(bool), &has_morph);
        if (has_morph) {
            walrus_rhi_set_texture(3, prim->morph_target);
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
            walrus_rhi_set_vertex_buffer(j, stream->buffer, stream->layout_handle, stream->offset,
                                         stream->num_vertices);
        }
        walrus_rhi_submit(0, s_data->depth_skin_shader, 0, WR_RHI_DISCARD_ALL);
        dump_stats(renderer, prim);
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
        if (setup_primitive(prim, WR_RHI_STATE_DEFAULT, true)) {
            walrus_rhi_submit(1, s_data->gbuffer_shader, 0, WR_RHI_DISCARD_ALL);
            dump_stats(renderer, prim);
        }
    }
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
        if (setup_primitive(prim, WR_RHI_STATE_DEFAULT, true)) {
            walrus_rhi_submit(1, s_data->gbuffer_skin_shader, 0, WR_RHI_DISCARD_ALL);
            dump_stats(renderer, prim);
        }
    }
}

void walrus_forward_renderer_submit_mesh(Walrus_DeferredRenderer *renderer, mat4 const world, Walrus_Mesh *mesh,
                                         Walrus_TransientBuffer weights)
{
    walrus_rhi_set_transform(world);

    if (weights.handle.id != WR_INVALID_HANDLE) {
        walrus_rhi_set_transient_buffer(0, &weights);
    }

    for (u32 i = 0; i < mesh->num_primitives; ++i) {
        Walrus_MeshPrimitive *prim = &mesh->primitives[i];
        if (setup_primitive(prim, WR_RHI_STATE_DEFAULT, false)) {
            walrus_rhi_submit(2, s_data->forward_shader, 0, WR_RHI_DISCARD_ALL);
            dump_stats(renderer, prim);
        }
    }
}

void walrus_forward_renderer_submit_skinned_mesh(Walrus_DeferredRenderer *renderer, mat4 const world, Walrus_Mesh *mesh,
                                                 Walrus_TransientBuffer joints, Walrus_TransientBuffer weights)
{
    walrus_rhi_set_transform(world);

    walrus_rhi_set_transient_buffer(0, &joints);
    if (weights.handle.id != WR_INVALID_HANDLE) {
        walrus_rhi_set_transient_buffer(1, &weights);
    }

    for (u32 i = 0; i < mesh->num_primitives; ++i) {
        Walrus_MeshPrimitive *prim = &mesh->primitives[i];
        if (setup_primitive(prim, WR_RHI_STATE_DEFAULT, false)) {
            walrus_rhi_submit(2, s_data->forward_skin_shader, 0, WR_RHI_DISCARD_ALL);
            dump_stats(renderer, prim);
        }
    }
}

void walrus_deferred_renderer_lighting(void)
{
    walrus_rhi_set_uniform(s_data->u_gpos, 0, sizeof(u32), &(u32){0});
    walrus_rhi_set_texture(0, walrus_rhi_get_texture(s_data->gbuffer, 0));

    walrus_rhi_set_uniform(s_data->u_gnormal, 0, sizeof(u32), &(u32){1});
    walrus_rhi_set_texture(1, walrus_rhi_get_texture(s_data->gbuffer, 1));

    walrus_rhi_set_uniform(s_data->u_galbedo, 0, sizeof(u32), &(u32){4});
    walrus_rhi_set_texture(4, walrus_rhi_get_texture(s_data->gbuffer, 4));

    walrus_rhi_set_uniform(s_data->u_gemissive, 0, sizeof(u32), &(u32){5});
    walrus_rhi_set_texture(5, walrus_rhi_get_texture(s_data->gbuffer, 5));

    walrus_rhi_set_uniform(s_data->u_gdepth, 0, sizeof(u32), &(u32){6});
    walrus_rhi_set_texture(6, walrus_rhi_get_texture(s_data->gbuffer, 6));

#if 1
    walrus_rhi_set_state(WR_RHI_STATE_WRITE_RGB | WR_RHI_STATE_WRITE_Z, 0);
#else
    walrus_rhi_set_state(WR_RHI_STATE_WRITE_RGB, 0);
#endif
    walrus_rhi_set_vertex_buffer(0, s_data->quad_vertices, s_data->quad_layout, 0, 4);
    walrus_rhi_set_index_buffer(s_data->quad_indices, 0, 6);
    walrus_rhi_submit(2, s_data->deferred_shader, 0, WR_RHI_DISCARD_ALL);
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
