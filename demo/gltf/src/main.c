#include <engine/engine.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/assert.h>
#include <engine/model.h>
#include <engine/imgui.h>
#include <rhi/rhi.h>
#include <engine/batch_renderer.h>
#include <engine/shader_library.h>
#include <engine/thread_pool.h>
#include <engine/animator.h>
#include <core/sys.h>
#include <core/array.h>

#include <cglm/cglm.h>
#include <string.h>

typedef struct {
    Walrus_Model         model;
    Walrus_Animator      animator;
    Walrus_ProgramHandle shader;
    Walrus_ProgramHandle skin_shader;
    mat4                 view;
    mat4                 projection;
    mat4                 world;
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
} AppData;

static void setup_texture_uniforms(AppData *data)
{
    u32 unit = 0;
    walrus_rhi_set_uniform(data->u_albedo, 0, sizeof(u32), &unit);
    unit = 1;
    walrus_rhi_set_uniform(data->u_emissive, 0, sizeof(u32), &unit);
    unit = 2;
    walrus_rhi_set_uniform(data->u_normal, 0, sizeof(u32), &unit);
    unit = 3;
    walrus_rhi_set_uniform(data->u_morph_texture, 0, sizeof(u32), &unit);
}

void walk(f32 value, void *userdata)
{
    walrus_trace("walk: %f", value);
}

void interact(void *userdata)
{
    walrus_trace("interact");
}

Walrus_AppError on_init(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);
    glm_mat4_identity(data->world);
    data->u_albedo          = walrus_rhi_create_uniform("u_albedo", WR_RHI_UNIFORM_SAMPLER, 1);
    data->u_albedo_factor   = walrus_rhi_create_uniform("u_albedo_factor", WR_RHI_UNIFORM_VEC4, 1);
    data->u_emissive        = walrus_rhi_create_uniform("u_emissive", WR_RHI_UNIFORM_SAMPLER, 1);
    data->u_emissive_factor = walrus_rhi_create_uniform("u_emissive_factor", WR_RHI_UNIFORM_VEC3, 1);
    data->u_normal          = walrus_rhi_create_uniform("u_normal", WR_RHI_UNIFORM_SAMPLER, 1);
    data->u_normal_scale    = walrus_rhi_create_uniform("u_normal_scale", WR_RHI_UNIFORM_FLOAT, 1);
    data->u_alpha_cutoff    = walrus_rhi_create_uniform("u_alpha_cutoff", WR_RHI_UNIFORM_FLOAT, 1);
    data->u_has_normal      = walrus_rhi_create_uniform("u_has_normal", WR_RHI_UNIFORM_BOOL, 1);
    data->u_morph_texture   = walrus_rhi_create_uniform("u_morph_texture", WR_RHI_UNIFORM_SAMPLER, 1);
    data->u_has_morph       = walrus_rhi_create_uniform("u_has_morph", WR_RHI_UNIFORM_BOOL, 1);

    Walrus_ShaderHandle vs      = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_mesh.glsl");
    Walrus_ShaderHandle vs_skin = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_skinned_mesh.glsl");
    Walrus_ShaderHandle fs      = walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_mesh.glsl");

    data->shader      = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs, fs}, 2, true);
    data->skin_shader = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_skin, fs}, 2, true);

    u32 rgba            = 0;
    data->black_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &rgba);
    rgba                = 0xffffffff;
    data->white_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &rgba);

    walrus_rhi_set_view_rect(0, 0, 0, 1440, 900);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0, 1.0, 0);

    glm_lookat((vec3){0, 2, 5}, (vec3){0, 0, 0}, (vec3){0, 1, 0}, data->view);
    glm_perspective(glm_rad(45), 1440.0 / 900.0, 0.1, 1000.0, data->projection);
    walrus_rhi_set_view_transform(0, data->view, data->projection);

    u64         ts       = walrus_sysclock(WR_SYS_CLOCK_UNIT_MILLSEC);
    char const *filename = "assets/gltf/shibahu/scene.gltf";
    if (walrus_model_load_from_file(&data->model, filename) != WR_MODEL_SUCCESS) {
        walrus_error("error loading model from: %s !", filename);
    }
    walrus_trace("model load time: %llu ms", walrus_sysclock(WR_SYS_CLOCK_UNIT_MILLSEC) - ts);

    setup_texture_uniforms(data);

    walrus_rhi_touch(0);

    walrus_animator_init(&data->animator);
    walrus_animator_bind(&data->animator, &data->model);
    walrus_animator_play(&data->animator, 0);

    Walrus_ControlMap *control = walrus_engine_control();
    walrus_control_bind_axis(control, "Walk", walk, NULL);
    walrus_control_bind_action(control, "Interact", interact, NULL);
    walrus_control_add_axis_button(control, "Walk", WR_INPUT_KEYBOARD, WR_KEY_W, 1, true);
    walrus_control_add_axis_button(control, "Walk", WR_INPUT_KEYBOARD, WR_KEY_S, -1, true);
    walrus_control_add_action_button(control, "Interact", WR_INPUT_KEYBOARD, WR_KEY_SPACE);
    walrus_control_add_action_button(control, "Interact", WR_INPUT_MOUSE, WR_MOUSE_BTN_LEFT);

    return WR_APP_SUCCESS;
}

static void node_callback(Walrus_ModelNode *node, void *userdata)
{
    AppData *data = userdata;

    if (node->skin) {
        Walrus_TransientBuffer buffer;
        if (walrus_rhi_alloc_transient_buffer(&buffer, node->skin->num_joints, sizeof(mat4))) {
            mat4 *m = (mat4 *)buffer.data;
            for (u32 i = 0; i < node->skin->num_joints; ++i) {
                walrus_animator_transform(&data->animator, node->skin->joints[i].node, m[i]);
                glm_mat4_mul(m[i], node->skin->joints[i].inverse_bind_matrix, m[i]);
            }
        }
        walrus_rhi_set_transient_buffer(0, &buffer);
    }

    if (node->mesh && node->mesh->num_weights > 0) {
        Walrus_TransientBuffer buffer;
        if (walrus_rhi_alloc_transient_buffer(&buffer, node->mesh->num_weights, sizeof(f32))) {
            walrus_animator_weights(&data->animator, node, (f32 *)buffer.data);
        }
        walrus_rhi_set_transient_buffer(1, &buffer);
    }
}

static void primitive_callback(Walrus_MeshPrimitive *prim, void *userdata)
{
    AppData             *data     = userdata;
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

        walrus_rhi_set_uniform(data->u_alpha_cutoff, 0, sizeof(f32), &alpha_cutoff);

        if (material->albedo) {
            walrus_rhi_set_texture(0, material->albedo->handle);
        }
        else {
            walrus_rhi_set_texture(0, data->black_texture);
        }
        walrus_rhi_set_uniform(data->u_albedo_factor, 0, sizeof(vec4), material->albedo_factor);

        if (material->emissive) {
            walrus_rhi_set_texture(1, material->emissive->handle);
        }
        else {
            walrus_rhi_set_texture(1, data->white_texture);
        }

        bool has_normal = material->normal != NULL;
        walrus_rhi_set_uniform(data->u_has_normal, 0, sizeof(bool), &has_normal);
        walrus_rhi_set_uniform(data->u_normal_scale, 0, sizeof(f32), &material->normal_scale);
        if (has_normal) {
            walrus_rhi_set_texture(2, material->normal->handle);
        }
        else {
            walrus_rhi_set_texture(2, data->black_texture);
        }
        walrus_rhi_set_uniform(data->u_emissive_factor, 0, sizeof(vec3), material->emissive_factor);

        bool has_morph = prim->morph_target.id != WR_INVALID_HANDLE;
        walrus_rhi_set_uniform(data->u_has_morph, 0, sizeof(bool), &has_morph);
        if (has_morph) {
            walrus_rhi_set_texture(3, prim->morph_target);
        }
    }
}

void on_render(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);
    u32      width, height;
    walrus_rhi_get_resolution(&width, &height);
    walrus_imgui_new_frame(width, height, 255);

    ImGuizmo_SetRect(0, 0, width, height);
    ImGuizmo_SetOrthographic(false);
    ImGuizmo_Manipulate(data->view[0], data->projection[0], TRANSLATE, WORLD, data->world[0], NULL, NULL, NULL, NULL);

    igBegin("Hello, world!", NULL, 0);
    igText("This is some useful text");
    igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
    static i32 lod = 0;
    igInputInt("LOD", &lod, 1, 1, 0);
    igImage(igHandle(data->model.textures[0].handle, IMGUI_TEXTURE_FLAGS_ALPHA_BLEND, lod), (ImVec2){100, 100},
            (ImVec2){0, 0}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){0});
    igEnd();
    walrus_imgui_end_frame();

    walrus_model_submit(0, &data->model, data->world, data->shader, data->skin_shader, 0, node_callback,
                        primitive_callback, data);
}

void on_tick(Walrus_App *app, f32 dt)
{
    AppData *data = walrus_app_userdata(app);
    glm_rotate(data->world, glm_rad(20.0) * dt, (vec3){0, 1, 0});
    walrus_animator_tick(&data->animator, dt);
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
