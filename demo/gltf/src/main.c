#include <engine/engine.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/assert.h>
#include <engine/model.h>
#include <rhi/rhi.h>
#include <engine/batch_renderer.h>

#include <cglm/cglm.h>
#include <string.h>

typedef struct {
    Walrus_Model         model;
    Walrus_ProgramHandle shader;
    mat4                 world;
    Walrus_UniformHandle u_albedo;
    Walrus_UniformHandle u_albedo_factor;
    Walrus_UniformHandle u_emissive;
    Walrus_UniformHandle u_emissive_factor;
    Walrus_TextureHandle black_texture;
    Walrus_TextureHandle white_texture;

    f32 debug_xoffset;
    f32 debug_yoffset;
} AppData;

char const *vs_src =
    "layout(location = 0) in vec3 a_pos;"
    "layout(location = 1) in vec3 a_normal;"
    "layout(location = 3) in vec2 a_uv;"
    "uniform mat4 u_viewproj;"
    "uniform mat4 u_model;"
    "out vec3 v_normal;"
    "out vec2 v_uv;"
    "void main() {"
    " gl_Position = u_viewproj * u_model * vec4(a_pos, 1);"
    " mat3 nmat = transpose(inverse(mat3(u_model))); "
    " v_normal = nmat * a_normal;"
    " v_uv = a_uv;"
    "}";

char const *fs_src =
    "out vec4 fragcolor;"
    "in vec3 v_normal;"
    "in vec2 v_uv;"
    "uniform sampler2D u_albedo;"
    "uniform vec4 u_albedo_factor;"
    "uniform sampler2D u_emissive;"
    "uniform vec3 u_emissive_factor;"
    "void main() {"
    " vec3 light_dir = normalize(vec3(0, 0, 1));"
    " float diff = max(dot(normalize(v_normal), light_dir), 0.0);"
    " vec3 emissive = texture(u_emissive, v_uv).rgb;"
    " vec4 albedo = texture(u_albedo, v_uv) * u_albedo_factor;"
    " fragcolor = vec4(diff * albedo.rgb + emissive, albedo.a);"
    " fragcolor = vec4(emissive, albedo.a);"
    "}";

Walrus_AppError on_init(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);
    glm_mat4_identity(data->world);
    data->u_albedo          = walrus_rhi_create_uniform("u_albedo", WR_RHI_UNIFORM_SAMPLER, 1);
    data->u_albedo_factor   = walrus_rhi_create_uniform("u_albedo_factor", WR_RHI_UNIFORM_VEC4, 1);
    data->u_emissive        = walrus_rhi_create_uniform("u_emissive", WR_RHI_UNIFORM_SAMPLER, 1);
    data->u_emissive_factor = walrus_rhi_create_uniform("u_emissive_factor", WR_RHI_UNIFORM_VEC3, 1);

    Walrus_ShaderHandle vs = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, vs_src);
    Walrus_ShaderHandle fs = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_src);
    data->shader           = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs, fs}, 2, true);
    walrus_rhi_destroy_shader(vs);
    walrus_rhi_destroy_shader(fs);

    u32 rgba            = 0;
    data->black_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &rgba);
    rgba                = 0xffffffff;
    data->white_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &rgba);

    walrus_rhi_set_view_rect(0, 0, 0, 1440, 900);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0, 1.0, 0);

    walrus_rhi_set_view_rect(1, 0, 0, 1440, 900);
    walrus_rhi_set_view_clear(1, WR_RHI_CLEAR_NONE, 0, 1.0, 0);

    mat4 view;
    mat4 projection;
    glm_lookat((vec3){0, 2, 5}, (vec3){0, 0, 0}, (vec3){0, 1, 0}, view);
    glm_perspective(glm_rad(45), 1440.0 / 900.0, 0.1, 1000.0, projection);
    walrus_rhi_set_view_transform(0, view, projection);

    glm_ortho(0, 1440, 900, 0, 0, 1000, projection);
    walrus_rhi_set_view_transform(1, GLM_MAT4_IDENTITY, projection);

    char const *filename = "assets/gltf/shibahu/scene.gltf";
    if (walrus_model_load_from_file(&data->model, filename) != WR_MODEL_SUCCESS) {
        walrus_error("error loading model from: %s !", filename);
    }

    walrus_rhi_frame();

    return WR_APP_SUCCESS;
}

static void submit_callback(Walrus_ModelNode *node, Walrus_MeshPrimitive *prim, void *userdata)
{
    AppData *data = userdata;
    mat4     world;
    walrus_transform_compose(&node->world_transform, world);
    glm_mat4_mul(data->world, world, world);
    walrus_rhi_set_transform(world);
    u64 flags = WR_RHI_STATE_DEFAULT | WR_RHI_STATE_BLEND_ALPHA;
    if (prim->material) {
        if (!prim->material->double_sided) {
            flags |= WR_RHI_STATE_CULL_CW;
        }
        walrus_rhi_set_state(flags, 0);

        if (prim->material->albedo) {
            u32 unit = 0;
            walrus_rhi_set_texture(unit, prim->material->albedo->handle);
            walrus_rhi_set_uniform(data->u_albedo, 0, sizeof(u32), &unit);
            /* walrus_batch_render_texture(prim->material->albedo->handle, */
            /*                             (vec3){data->debug_xoffset, data->debug_yoffset, 0}, (versor){0, 0, 0, 1}, */
            /*                             (vec3){100, 100, 0}, 0xffffffff, 0, 0, 0); */
        }
        else {
            u32 unit = 0;
            walrus_rhi_set_texture(unit, data->black_texture);
            walrus_rhi_set_uniform(data->u_albedo, 0, sizeof(u32), &unit);
        }
        walrus_rhi_set_uniform(data->u_albedo_factor, 0, sizeof(vec4), prim->material->albedo_factor);

        if (prim->material->emissive) {
            u32 unit = 1;
            walrus_rhi_set_texture(unit, prim->material->emissive->handle);
            walrus_rhi_set_uniform(data->u_emissive, 0, sizeof(u32), &unit);
            /* walrus_batch_render_texture(data->model.materials[id].emissive->handle, */
            /*                             (vec3){data->debug_xoffset, data->debug_yoffset + 100, 0}, (versor){0, 0, 0, 1}, */
            /*                             (vec3){100, 100, 0}, 0xffffffff, 0, 0, 0); */
            data->debug_xoffset += 100;
        }
        else {
            u32 unit = 1;
            walrus_rhi_set_texture(unit, data->white_texture);
            walrus_rhi_set_uniform(data->u_emissive, 0, sizeof(u32), &unit);
        }
        walrus_rhi_set_uniform(data->u_emissive_factor, 0, sizeof(vec3), prim->material->emissive_factor);
    }
}

void on_render(Walrus_App *app)
{
    /* walrus_batch_render_begin(1, WR_RHI_STATE_WRITE_RGB | WR_RHI_STATE_WRITE_A | WR_RHI_STATE_BLEND_ALPHA); */
    AppData *data       = walrus_app_userdata(app);
    data->debug_xoffset = 50;
    data->debug_yoffset = 600;
    walrus_model_submit(0, &data->model, data->shader, 0, submit_callback, data);
    /* walrus_batch_render_end(); */
}

void on_tick(Walrus_App *app, f32 dt)
{
    AppData *data = walrus_app_userdata(app);
    glm_rotate(data->world, glm_rad(20.0) * dt, (vec3){0, 1, 0});
}

void on_shutdown(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);
    walrus_model_shutdown(&data->model);
}

int main(void)
{
    Walrus_App *app = walrus_app_create(walrus_new(AppData, 1));
    walrus_app_set_init(app, on_init);
    walrus_app_set_render(app, on_render);
    walrus_app_set_tick(app, on_tick);
    walrus_app_set_shutdown(app, on_shutdown);

    walrus_engine_init_run("gltf", 1440, 900, app);

    return 0;
}
