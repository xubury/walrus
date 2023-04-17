#include "game_state.h"

#include <core/log.h>
#include <core/memory.h>
#include <core/ray.h>
#include <engine/engine.h>

#include "hex.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

char const *ins_hex_src =
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
    "vec3 linear_to_srgb(vec3 linear)\n"
    "{\n"
    "    bvec3 cutoff = lessThan(linear, vec3(0.0031308));\n"
    "    vec3 higher = vec3(1.055) * pow(linear, vec3(1.0 / 2.4)) - vec3(0.055);\n"
    "    vec3 lower = linear * vec3(12.92);\n"
    "    return mix(higher, lower, cutoff);\n"
    "}\n"
    "void main() {\n"
    "    vec3 color = texture(u_texture, v_uv).rgb;\n"
    "    color = linear_to_srgb(color) * vec3(0.529411, 0.756862, 1.0);\n"
    "    frag_color = vec4(color, 1.0);\n"
    "}\n";

char const *fs_grid_src =
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = vec4(0.2);\n"
    "}\n";

void game_state_init(Romantik_GameState *state)
{
    Walrus_Window *window = walrus_engine_window();
    i32 const      width  = walrus_window_width(window);
    i32 const      height = walrus_window_height(window);

    camera_init(&state->cam);
    romantik_game_init(&state->game);

    walrus_rhi_set_view_rect(0, 0, 0, width, height);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0xffd580ff, 1.0, 0);

    mat4 projection;
    glm_perspective(glm_rad(45.0), (float)width / height, 0.1, 1000.0, projection);
    walrus_rhi_set_view_transform(0, NULL, projection);

    struct {
        vec3 pos;
        vec2 uv;
    } vertices[6];
    f32 rad = glm_rad(90);
    for (u8 i = 0; i < 6; ++i) {
        vertices[i].pos[0] = cos(rad);
        vertices[i].pos[1] = 0;
        vertices[i].pos[2] = sin(rad);
        vertices[i].uv[0]  = (cos(rad) + 1) * 0.5;
        vertices[i].uv[1]  = (sin(rad) + 1) * 0.5;
        rad += glm_rad(60);
    }
    u16 indices[]       = {0, 1, 2, 2, 3, 4, 4, 5, 0, 0, 2, 4};
    state->buffer       = walrus_rhi_create_buffer(vertices, sizeof(vertices), 0);
    state->index_buffer = walrus_rhi_create_buffer(indices, sizeof(indices), WR_RHI_BUFFER_INDEX);

    Walrus_VertexLayout layout;
    walrus_vertex_layout_begin(&layout);
    walrus_vertex_layout_add(&layout, 0, 3, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_add(&layout, 1, 2, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_end(&layout);
    state->layout = walrus_rhi_create_vertex_layout(&layout);

    walrus_vertex_layout_begin_instance(&layout, 1);
    walrus_vertex_layout_add(&layout, 2, 4, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_add(&layout, 3, 4, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_add(&layout, 4, 4, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_add(&layout, 5, 4, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_end(&layout);
    state->model_layout = walrus_rhi_create_vertex_layout(&layout);

    Romantik_Game *game = &state->game;
    /* romantik_set_avail(game, 0, 0); */
    for (i8 q = -5; q <= 5; ++q) {
        for (i8 r = -5; r <= 5; ++r) {
            u32 dist = hex_distance(q, r, 0, 0);
            if (dist <= 2) {
                romantik_set_avail(game, q, r);
            }
        }
    }

    state->placed_buffer = walrus_rhi_create_buffer(NULL, 1000 * sizeof(mat4), 0);
    state->avail_buffer  = walrus_rhi_create_buffer(NULL, 2000 * sizeof(mat4), 0);

    mat4 *model = walrus_new(mat4, game->num_avail_grids);
    hex_map_compute_models(&game->map, model, game->num_avail_grids, HEX_FLAG_AVAIL);
    walrus_rhi_update_buffer(state->avail_buffer, 0, game->num_avail_grids * sizeof(mat4), model);
    walrus_free(model);

    state->u_texture = walrus_rhi_create_uniform("u_texture", WR_RHI_UNIFORM_SAMPLER, 1);

    Walrus_ShaderHandle vs      = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, hex_src);
    Walrus_ShaderHandle vs_ins  = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, ins_hex_src);
    Walrus_ShaderHandle fs      = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_src);
    Walrus_ShaderHandle fs_grid = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_grid_src);
    state->pick_shader          = walrus_rhi_create_program(vs, fs);
    state->map_shader           = walrus_rhi_create_program(vs_ins, fs);
    state->grid_shader          = walrus_rhi_create_program(vs_ins, fs_grid);

    state->hide_picker = true;

    glm_mat4_identity(state->model);

    stbi_set_flip_vertically_on_load(true);
    i32 x, y, c;
    u8 *img = stbi_load("imgs/test.png", &x, &y, &c, 4);
    if (img != NULL) {
        walrus_trace("load image width: %d height: %d channel: %d", x, y, c);

        state->texture = walrus_rhi_create_texture2d(
            x, y, WR_RHI_FORMAT_RGBA8, 0, WR_RHI_SAMPLER_MIN_LINEAR | WR_RHI_SAMPLER_MIP_LINEAR | WR_RHI_TEXTURE_SRGB,
            img, x * y * 4);

        stbi_image_free(img);
    }
    else {
        walrus_error("fail to load image: %s", stbi_failure_reason());
    }
}

void game_state_tick(Romantik_GameState *state, f32 dt)
{
    camera_tick(&state->cam, dt);
    Walrus_Input  *input = walrus_engine_input();
    Romantik_Game *game  = &state->game;
    HexMap        *map   = &game->map;
    state->hide_picker   = true;
    if (!walrus_input_down(input->mouse, WR_MOUSE_BTN_RIGHT)) {
        vec2 axis;
        vec3 world_pos, world_dir;
        vec3 select;
        walrus_input_axis(input->mouse, WR_MOUSE_AXIS_CURSOR, &axis[0], &axis[1], NULL);
        walrus_rhi_screen_to_world(0, axis, world_pos);
        walrus_rhi_screen_to_world_dir(0, axis, world_dir);
        if (walrus_intersect_ray_plane(select, world_pos, world_dir, (vec3 const){0, 1, 0}, (vec3 const){0, 0, 0})) {
            i32 q, r;
            hex_pixel_to_qr(map->hex_size, select[0], select[2], &q, &r);
            if (hex_map_test_flags(map, q, r, HEX_FLAG_AVAIL)) {
                hex_map_compute_model(map, state->model, q, r);
                state->hide_picker = false;
                if (walrus_input_pressed(input->mouse, WR_MOUSE_BTN_LEFT)) {
                    if (romantik_place_grid(game, q, r)) {
                        walrus_rhi_update_buffer(state->placed_buffer, (game->num_placed_grids - 1) * sizeof(mat4),
                                                 sizeof(mat4), &state->model);

                        mat4 *models = walrus_new(mat4, game->num_avail_grids);
                        hex_map_compute_models(map, models, game->num_avail_grids, HEX_FLAG_AVAIL);
                        walrus_rhi_update_buffer(state->avail_buffer, 0, game->num_avail_grids * sizeof(mat4), models);
                        walrus_free(models);
                    }
                }
            }
        }
    }
}
