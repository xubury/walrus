#include <core/type.h>
#include <core/sys.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/handle_alloc.h>
#include <core/memory.h>
#include <core/string.h>
#include <core/hash.h>
#include <core/image.h>

#include <engine/engine.h>
#include <rhi/rhi.h>

#include <math.h>
#include <string.h>

#include <cglm/cglm.h>

char const *ins_hex_src =
    "layout (location = 0) in vec3 a_pos;\n"
    "layout (location = 1) in vec2 a_uv;\n"
    "out vec2 v_pos;\n"
    "out vec2 v_uv;\n"
    "uniform mat4 u_viewproj;\n"
    "uniform mat4 u_model;\n"
    "void main() { \n"
    "    gl_Position = u_viewproj * u_model * vec4(a_pos, 1.0);\n"
    "    v_pos = a_pos.xy;\n"
    "    v_uv = a_uv;\n"
    "}\n";

char const *layer_fs_src =
    "out vec4 fragColor;"
    "in vec2 v_pos;"
    "in vec2 v_uv;"
    "uniform sampler2D u_texture;"
    "void main() { "
    "    fragColor = texture(u_texture, v_uv) * vec4(v_pos, 1.0, 1.0);"
    "}";

typedef struct {
    Walrus_ProgramHandle map_shader;
    Walrus_UniformHandle u_texture;
    Walrus_BufferHandle  buffer;
    Walrus_BufferHandle  uv_buffer;
    Walrus_BufferHandle  index_buffer;
    Walrus_LayoutHandle  layout;
    Walrus_LayoutHandle  uv_layout;
    Walrus_TextureHandle font;

    mat4 model;

} AppData;

void on_render(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);

    walrus_rhi_set_transform(data->model);
    walrus_rhi_set_vertex_buffer(0, data->buffer, data->layout, 0, UINT32_MAX);
    walrus_rhi_set_vertex_buffer(1, data->uv_buffer, data->uv_layout, 0, UINT32_MAX);
    walrus_rhi_set_index_buffer(data->index_buffer, 0, UINT32_MAX);
    u32 unit = 0;
    walrus_rhi_set_texture(unit, data->font);
    walrus_rhi_set_uniform(data->u_texture, 0, sizeof(u32), &unit);
    /* walrus_rhi_set_image(0, data->texture, 0, WR_RHI_ACCESS_READ, WR_RHI_FORMAT_RGBA8); */
    walrus_rhi_submit(0, data->map_shader, 0, WR_RHI_DISCARD_ALL);
}

void on_tick(Walrus_App *app, float dt)
{
    AppData *data = walrus_app_userdata(app);
    glm_rotate(data->model, 1.0 * dt, (vec3){0, 1, 0});
}

void on_event(Walrus_App *app, Walrus_Event *e)
{
    walrus_unused(app);

    if (e->type == WR_EVENT_TYPE_BUTTON) {
        if (e->button.device == WR_INPUT_KEYBOARD && e->button.button == WR_KEY_ESCAPE) {
            walrus_engine_exit();
        }
    }
}

Walrus_AppError on_init(Walrus_App *app)
{
    Walrus_AppError err      = WR_APP_SUCCESS;
    AppData        *app_data = walrus_app_userdata(app);
    Walrus_Window  *window   = walrus_engine_window();
    i32 const       width    = walrus_window_width(window);
    i32 const       height   = walrus_window_height(window);

    walrus_rhi_set_view_rect(0, 0, 0, width, height);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0xffffffff, 1.0, 0);

    mat4 projection;
    mat4 view = GLM_MAT4_IDENTITY_INIT;
    glm_perspective(glm_rad(45.0), (float)width / height, 0.1, 100, projection);
    walrus_rhi_set_view_transform(0, view, projection);

    // clang-format off
    f32 vertices[] = {
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
    };
    f32 uvs[] = {
         0.0f, 0.0f,
         1.0f, 0.0f,
         1.0f, 1.0f,
         1.0f, 1.0f,
         0.0f, 1.0f,
         0.0f, 0.0f,

         0.0f, 0.0f,
         1.0f, 0.0f,
         1.0f, 1.0f,
         1.0f, 1.0f,
         0.0f, 1.0f,
         0.0f, 0.0f,

         1.0f, 0.0f,
         1.0f, 1.0f,
         0.0f, 1.0f,
         0.0f, 1.0f,
         0.0f, 0.0f,
         1.0f, 0.0f,

         1.0f, 0.0f,
         1.0f, 1.0f,
         0.0f, 1.0f,
         0.0f, 1.0f,
         0.0f, 0.0f,
         1.0f, 0.0f,

         0.0f, 1.0f,
         1.0f, 1.0f,
         1.0f, 0.0f,
         1.0f, 0.0f,
         0.0f, 0.0f,
         0.0f, 1.0f,

         0.0f, 1.0f,
         1.0f, 1.0f,
         1.0f, 0.0f,
         1.0f, 0.0f,
         0.0f, 0.0f,
         0.0f, 1.0f
    };
    // clang-format on

    Walrus_VertexLayout layout;
    walrus_vertex_layout_begin(&layout);
    walrus_vertex_layout_add(&layout, 0, 3, WR_RHI_COMPONENT_FLOAT, false);
    walrus_vertex_layout_end(&layout);
    app_data->layout = walrus_rhi_create_vertex_layout(&layout);
    app_data->layout = walrus_rhi_create_vertex_layout(&layout);

    walrus_vertex_layout_begin(&layout);
    walrus_vertex_layout_add(&layout, 1, 2, WR_RHI_COMPONENT_FLOAT, false);
    walrus_vertex_layout_end(&layout);
    app_data->uv_layout = walrus_rhi_create_vertex_layout(&layout);

    u16 indices[] = {
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    };
    app_data->buffer       = walrus_rhi_create_buffer(vertices, sizeof(vertices), 0);
    app_data->uv_buffer    = walrus_rhi_create_buffer(uvs, sizeof(uvs), 0);
    app_data->index_buffer = walrus_rhi_create_buffer(indices, sizeof(indices), WR_RHI_BUFFER_INDEX);

    app_data->u_texture = walrus_rhi_create_uniform("u_texture", WR_RHI_UNIFORM_SAMPLER, 1);

    Walrus_ShaderHandle vs = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, ins_hex_src);
    Walrus_ShaderHandle fs = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, layer_fs_src);
    app_data->map_shader   = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs, fs}, 2, true);

    glm_mat4_identity(app_data->model);
    glm_translate(app_data->model, (vec3){0, 0, -2});

    u64          ts = walrus_sysclock(WR_SYS_CLOCK_UNIT_MILLSEC);
    Walrus_Image image;
    walrus_image_load_from_file(&image, "assets/imgs/0.png");
    walrus_trace("stbi_load time: %llu ms", walrus_sysclock(WR_SYS_CLOCK_UNIT_MILLSEC) - ts);
    if (image.data != NULL) {
        walrus_trace("load image width: %d height: %d", image.width, image.height);

        app_data->font = walrus_rhi_create_texture2d(
            image.width, image.height, WR_RHI_FORMAT_RGBA8, 0,
            WR_RHI_SAMPLER_MIN_LINEAR | WR_RHI_SAMPLER_MIP_LINEAR | WR_RHI_TEXTURE_SRGB, image.data);

        walrus_image_shutdown(&image);
    }
    else {
        err = WR_APP_INIT_FAIL;

        walrus_error("fail to load image");
    }

    return err;
}

void on_shutdown(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);
    walrus_rhi_destroy_uniform(data->u_texture);
}

int main(int argc, char *argv[])
{
    walrus_unused(argc);
    walrus_unused(argv);

    Walrus_App *app = walrus_app_create(walrus_malloc(sizeof(AppData)));
    walrus_app_set_init(app, on_init);
    walrus_app_set_shutdown(app, on_shutdown);
    walrus_app_set_tick(app, on_tick);
    walrus_app_set_render(app, on_render);
    walrus_app_set_event(app, on_event);

    walrus_engine_init_run("Rotate Cube", 640, 480, app);

    return 0;
}
