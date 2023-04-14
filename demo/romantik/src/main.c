#include <core/macro.h>
#include <core/rect.h>
#include <core/math.h>
#include <core/memory.h>
#include <engine/engine.h>
#include <rhi/rhi.h>
#include <core/ray.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "camera.h"
#include "hex.h"
#include "hex_map.h"

char const *hex_src =
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

char const *hex_instance_src =
    "layout (location = 0) in vec3 a_pos;\n"
    "layout (location = 1) in vec2 a_uv;\n"
    "layout (location = 2) in mat4 a_model;\n"
    "out vec2 v_pos;\n"
    "out vec2 v_uv;\n"
    "uniform mat4 u_viewproj;\n"
    "void main() { \n"
    "    gl_Position = u_viewproj * a_model * vec4(a_pos, 1.0);\n"
    "    v_pos = a_pos.xy;\n"
    "    v_uv = a_uv;\n"
    "}\n";

char const *fs_src =
    "out vec4 frag_color;\n"
    "in vec2 v_pos;\n"
    "in vec2 v_uv;\n"
    "uniform sampler2D u_texture;\n"
    "vec3 acesToneMapping(vec3 color)"
    "{"
    "    const float a = 2.51f;"
    "    const float b = 0.03f;"
    "    const float c = 2.43f;"
    "    const float d = 0.59f;"
    "    const float e = 0.14f;"
    "    color = color;"
    "    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0, 1);"
    "}"
    "vec3 linear_to_srgb(vec3 linear)"
    "{"
    "    bvec3 cutoff = lessThan(linear, vec3(0.0031308)); "
    "    vec3 higher = vec3(1.055) * pow(linear, vec3(1.0 / 2.4)) - vec3(0.055);"
    "    vec3 lower = linear * vec3(12.92);"
    "    return mix(higher, lower, cutoff);"
    "}"
    "void main() { \n"
    "    vec3 color = texture(u_texture, v_uv).rgb; \n"
    "    color = linear_to_srgb(color) * vec3(0.529411, 0.756862, 1.0);\n"
    "    // color = acesToneMapping(color);\n"
    "    frag_color = vec4(color, 1.0);\n"
    "}\n";

typedef struct {
    Walrus_ProgramHandle pick_shader;
    Walrus_ProgramHandle map_shader;
    Walrus_UniformHandle u_texture;
    Walrus_BufferHandle  buffer;
    Walrus_BufferHandle  uv_buffer;
    Walrus_BufferHandle  model_buffer;
    Walrus_BufferHandle  index_buffer;
    Walrus_LayoutHandle  pos_layout;
    Walrus_LayoutHandle  uv_layout;
    Walrus_LayoutHandle  model_layout;
    Walrus_TextureHandle texture;

    mat4 model;

    CameraData cam;

    HexMap hex_map;
} AppData;

void on_render(Walrus_App *app)
{
    AppData    *data = walrus_app_userdata(app);
    CameraData *cam  = &data->cam;

    walrus_rhi_set_view_transform(0, cam->view, NULL);

    walrus_rhi_set_vertex_buffer(0, data->buffer, data->pos_layout, 0, UINT32_MAX);
    walrus_rhi_set_vertex_buffer(1, data->uv_buffer, data->uv_layout, 0, UINT32_MAX);
    walrus_rhi_set_index_buffer(data->index_buffer, 0, UINT32_MAX);
    walrus_rhi_set_texture(0, data->u_texture, data->texture);
    walrus_rhi_set_instance_buffer(data->model_buffer, data->model_layout, 0, UINT32_MAX);
    walrus_rhi_submit(0, data->map_shader, WR_RHI_DISCARD_INSTANCE_DATA);

    walrus_rhi_set_transform(data->model);
    walrus_rhi_submit(0, data->pick_shader, WR_RHI_DISCARD_ALL);
}

void on_tick(Walrus_App *app, float dt)
{
    AppData *data = walrus_app_userdata(app);

    camera_tick(&data->cam, dt);

    Walrus_Input *input = walrus_engine_input();
    if (!walrus_input_any_down(input->mouse)) {
        vec2 axis;
        vec3 world_pos, world_dir;
        vec3 select;
        walrus_input_axis(input->mouse, WR_MOUSE_AXIS_CURSOR, &axis[0], &axis[1], NULL);
        walrus_rhi_screen_to_world(0, axis, world_pos);
        walrus_rhi_screen_to_world_dir(0, axis, world_dir);
        if (walrus_intersect_ray_plane(select, world_pos, world_dir, (vec3 const){0, 1, 0}, (vec3 const){0, 0, 0})) {
            hex_map_compute_model_pixel(&data->hex_map, data->model, select[0], select[2]);
        }
    }
}

void on_event(Walrus_App *app, Walrus_Event *e)
{
    walrus_unused(app);

    if (e->type == WR_EVENT_TYPE_RESOLUTION) {
        walrus_rhi_set_view_rect(0, 0, 0, e->resolution.width, e->resolution.height);
        mat4 projection;
        glm_perspective(glm_rad(45.0), (float)e->resolution.width / e->resolution.height, 0.1, 1000.0, projection);
        walrus_rhi_set_view_transform(0, NULL, projection);
    }
    else if (e->type == WR_EVENT_TYPE_BUTTON) {
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

    camera_init(&app_data->cam);
    hex_map_init(&app_data->hex_map, 1, 10, 10);

    walrus_rhi_set_view_rect(0, 0, 0, width, height);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0xffd580ff, 1.0, 0);

    mat4 projection;
    glm_perspective(glm_rad(45.0), (float)width / height, 0.1, 1000.0, projection);
    walrus_rhi_set_view_transform(0, NULL, projection);

    vec3 vertices[6];
    vec2 uvs[6];
    f32  rad = glm_rad(90);
    for (u8 i = 0; i < 6; ++i) {
        vertices[i][0] = cos(rad);
        vertices[i][1] = 0;
        vertices[i][2] = sin(rad);
        uvs[i][0]      = (cos(rad) + 1) * 0.5;
        uvs[i][1]      = (sin(rad) + 1) * 0.5;
        rad += glm_rad(60);
    }

    Walrus_VertexLayout layout;
    walrus_vertex_layout_begin(&layout);
    walrus_vertex_layout_add(&layout, 0, 3, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_end(&layout);
    app_data->pos_layout = walrus_rhi_create_vertex_layout(&layout);

    walrus_vertex_layout_begin(&layout);
    walrus_vertex_layout_add(&layout, 1, 2, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_end(&layout);
    app_data->uv_layout = walrus_rhi_create_vertex_layout(&layout);

    walrus_vertex_layout_begin_instance(&layout, 1);
    walrus_vertex_layout_add(&layout, 2, 4, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_add(&layout, 3, 4, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_add(&layout, 4, 4, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_add(&layout, 5, 4, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_end(&layout);
    app_data->model_layout = walrus_rhi_create_vertex_layout(&layout);

    u16 indices[]       = {0, 1, 2, 2, 3, 4, 4, 5, 0, 0, 2, 4};
    app_data->buffer    = walrus_rhi_create_buffer(vertices, sizeof(vertices), 0);
    app_data->uv_buffer = walrus_rhi_create_buffer(uvs, sizeof(uvs), 0);
    mat4 models[2];
    hex_map_set_flags(&app_data->hex_map, 0, 0, HEX_FLAG_NORMAL);
    hex_map_set_flags(&app_data->hex_map, 1, 1, HEX_FLAG_NORMAL);
    hex_map_compute_models(&app_data->hex_map, models, 2, HEX_FLAG_NORMAL);

    app_data->model_buffer = walrus_rhi_create_buffer(models, sizeof(models), 0);
    app_data->index_buffer = walrus_rhi_create_buffer(indices, sizeof(indices), WR_RHI_BUFFER_INDEX);

    app_data->u_texture = walrus_rhi_create_uniform("u_texture", WR_RHI_UNIFORM_SAMPLER, 1);

    Walrus_ShaderHandle vs          = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, hex_src);
    Walrus_ShaderHandle vs_instance = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, hex_instance_src);
    Walrus_ShaderHandle fs          = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_src);
    app_data->pick_shader           = walrus_rhi_create_program(vs, fs);
    app_data->map_shader            = walrus_rhi_create_program(vs_instance, fs);

    glm_mat4_identity(app_data->model);

    stbi_set_flip_vertically_on_load(true);
    i32 x, y, c;
    u8 *img = stbi_load("imgs/test.png", &x, &y, &c, 4);
    if (img != NULL) {
        walrus_trace("load image width: %d height: %d channel: %d", x, y, c);

        app_data->texture = walrus_rhi_create_texture2d(
            x, y, WR_RHI_FORMAT_RGBA8, 0, WR_RHI_SAMPLER_MIN_LINEAR | WR_RHI_SAMPLER_MIP_LINEAR | WR_RHI_TEXTURE_SRGB,
            img, x * y * 4);

        stbi_image_free(img);
    }
    else {
        err = WR_APP_INIT_FAIL;

        walrus_error("fail to load image: %s", stbi_failure_reason());
    }

    return err;
}

int main(void)
{
    Walrus_EngineOption opt;
    opt.window_title  = "romantik";
    opt.window_width  = 1440;
    opt.window_height = 900;
    opt.window_flags  = WR_WINDOW_FLAG_VSYNC | WR_WINDOW_FLAG_OPENGL;
    opt.minfps        = 30.f;

    Walrus_App *app = walrus_app_create(walrus_malloc(sizeof(AppData)));
    walrus_app_set_init(app, on_init);
    walrus_app_set_tick(app, on_tick);
    walrus_app_set_render(app, on_render);
    walrus_app_set_event(app, on_event);

    walrus_engine_init_run(&opt, app);

    return 0;
}
