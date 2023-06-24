#include <engine/deferred_renderer.h>
#include <engine/shader_library.h>
#include <core/memory.h>
#include <core/macro.h>
#include <rhi/rhi.h>

#include <cglm/cglm.h>
#include <string.h>

typedef struct {
    Walrus_ProgramHandle shader;
    Walrus_ProgramHandle skin_shader;

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
    s_data                    = walrus_new(RenderData, 1);
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

    Walrus_ShaderHandle vs      = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_mesh.glsl");
    Walrus_ShaderHandle vs_skin = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_skinned_mesh.glsl");
    Walrus_ShaderHandle fs      = walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_mesh.glsl");

    s_data->shader      = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs, fs}, 2, true);
    s_data->skin_shader = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_skin, fs}, 2, true);

    u32 rgba              = 0;
    s_data->black_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &rgba);
    rgba                  = 0xffffffff;
    s_data->white_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &rgba);

    setup_texture_uniforms();

    walrus_rhi_touch(0);
}

static void node_callback(Walrus_Model const *model, Walrus_ModelNode const *node, void *userdata)
{
    Walrus_Animator const *animator = userdata;

    if (node->skin) {
        Walrus_TransientBuffer buffer;
        if (walrus_rhi_alloc_transient_buffer(&buffer, node->skin->num_joints, sizeof(mat4))) {
            mat4 *m = (mat4 *)buffer.data;
            for (u32 i = 0; i < node->skin->num_joints; ++i) {
                if (animator) {
                    walrus_animator_transform(animator, model, node->skin->joints[i].node, m[i]);
                }
                else {
                    walrus_transform_compose(&node->skin->joints[i].node->world_transform, m[i]);
                }
                glm_mat4_mul(m[i], node->skin->joints[i].inverse_bind_matrix, m[i]);
            }
        }
        walrus_rhi_set_transient_buffer(0, &buffer);
    }

    if (node->mesh && node->mesh->num_weights > 0) {
        Walrus_TransientBuffer buffer;
        if (walrus_rhi_alloc_transient_buffer(&buffer, node->mesh->num_weights, sizeof(f32))) {
            if (animator) {
                walrus_animator_weights(animator, model, node, (f32 *)buffer.data);
            }
            else {
                memcpy(buffer.data, node->mesh->weights, sizeof(f32) * node->mesh->num_weights);
            }
        }
        walrus_rhi_set_transient_buffer(1, &buffer);
    }
}

static void primitive_callback(Walrus_MeshPrimitive const *prim, void *userdata)
{
    walrus_unused(userdata);
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
}

void walrus_deferred_renderer_set_camera(Walrus_DeferredRenderer *renderer, Walrus_Camera *camera)
{
    walrus_rhi_set_view_rect(0, renderer->x, renderer->y, renderer->width, renderer->height);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0, 1.0, 0);
    walrus_rhi_set_view_transform(0, camera->view, camera->projection);
    walrus_rhi_set_framebuffer(0, renderer->framebuffer);
}

void walrus_deferred_renderer_submit(Walrus_Transform *transform, Walrus_Model *model, Walrus_Animator const *animator)
{
    mat4 world;
    walrus_transform_compose(transform, world);
    walrus_model_submit(0, model, world, s_data->shader, s_data->skin_shader, 0, node_callback, primitive_callback,
                        (void *)animator);
}
