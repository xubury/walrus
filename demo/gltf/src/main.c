#include <engine/engine.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/assert.h>
#include <engine/model.h>
#include <rhi/rhi.h>

#include <cglm/cglm.h>
#include <string.h>

typedef struct {
    Walrus_Model         model;
    Walrus_ProgramHandle shader;
    mat4                 world;
} AppData;

char const *vs_src =
    "layout(location = 0) in vec3 a_pos;"
    "layout(location = 1) in vec3 a_normal;"
    "uniform mat4 u_viewproj;"
    "uniform mat4 u_model;"
    "out vec3 v_normal;"
    "void main() {"
    " gl_Position = u_viewproj * u_model * vec4(a_pos, 1);"
    " mat3 nmat = transpose(inverse(mat3(u_model))); "
    " v_normal = nmat * a_normal;"
    "}";

char const *fs_src =
    "out vec4 fragcolor;"
    "in vec3 v_normal;"
    "void main() {"
    " vec3 light_dir = normalize(vec3(0, 1, -1));"
    " float diff = max(dot(normalize(v_normal), light_dir), 0.0);"
    " fragcolor = vec4(diff * vec3(1), 1);"
    "}";

Walrus_AppError on_init(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);
    glm_mat4_identity(data->world);
    Walrus_ShaderHandle vs = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, vs_src);
    Walrus_ShaderHandle fs = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_src);
    data->shader           = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs, fs}, 2, true);
    walrus_rhi_destroy_shader(vs);
    walrus_rhi_destroy_shader(fs);

    walrus_rhi_set_view_rect(0, 0, 0, 1440, 900);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0, 1.0, 0);

    mat4 view;
    mat4 projection;
    glm_lookat((vec3){0, 0, 5}, (vec3){0, 0, 0}, (vec3){0, 1, 0}, view);
    glm_perspective(glm_rad(45), 1440.0 / 900.0, 0.1, 1000.0, projection);
    walrus_rhi_set_view_transform(0, view, projection);

    char const *filename = "assets/gltf/shibahu/scene.gltf";
    if (walrus_model_load_from_file(&data->model, filename) != WR_MODEL_SUCCESS) {
        walrus_error("error loading model from: %s !", filename);
    }

    walrus_rhi_frame();

    return WR_APP_SUCCESS;
}

static void submit_callback(Walrus_ModelNode *node, Walrus_MeshPrimitive *prim, void *userdata)
{
    mat4 world;
    walrus_transform_compose(&node->world_transform, world);
    glm_mat4_mul(userdata, world, world);
    walrus_rhi_set_transform(world);
    if (!prim->material.double_sided) {
        walrus_rhi_set_state(WR_RHI_STATE_DEFAULT | WR_RHI_STATE_CULL_CW, 0);
    }
}

void on_render(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);
    walrus_model_submit(0, &data->model, data->shader, 0, submit_callback, data->world);
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
