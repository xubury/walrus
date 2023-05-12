#include "game_state.h"
#include "hex.h"

#include <core/log.h>
#include <core/memory.h>
#include <core/ray.h>
#include <core/macro.h>
#include <engine/engine.h>

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
    "layout (location = 2) in vec3 a_color;\n"
    "layout (location = 3) in float a_layer;\n"
    "layout (location = 4) in mat4 a_model;\n"
    "out vec2 v_pos;\n"
    "out vec2 v_uv;\n"
    "out vec3 v_color;\n"
    "out float v_layer;\n"
    "uniform mat4 u_viewproj;\n"
    "void main() { \n"
    "    gl_Position = u_viewproj * a_model * vec4(a_pos, 1.0);\n"
    "    v_pos = a_pos.xy;\n"
    "    v_uv = a_uv;\n"
    "    v_color = a_color;\n"
    "    v_layer = a_layer;\n"
    "}\n";

char const *layer_fs_src =
    "out vec4 frag_color;\n"
    "in vec2 v_pos;\n"
    "in vec2 v_uv;\n"
    "in float v_layer;\n"
    "in vec3 v_color;\n"
    "uniform sampler2DArray u_texture;\n"
    "vec3 linear_to_srgb(vec3 linear)\n"
    "{\n"
    "    bvec3 cutoff = lessThan(linear, vec3(0.0031308));\n"
    "    vec3 higher = vec3(1.055) * pow(linear, vec3(1.0 / 2.4)) - vec3(0.055);\n"
    "    vec3 lower = linear * vec3(12.92);\n"
    "    return mix(higher, lower, cutoff);\n"
    "}\n"
    "void main() {\n"
    "    vec3 color = texture(u_texture, vec3(v_uv, v_layer)).rgb;\n"
    "    color = linear_to_srgb(color);\n"
    "    frag_color = vec4(color, 1.0) * vec4(v_color, 1.0);\n"
    "}\n";

char const *fs_src =
    "out vec4 frag_color;\n"
    "in vec2 v_pos;\n"
    "in vec2 v_uv;\n"
    "uniform sampler2DArray u_texture;\n"
    "uniform vec4 u_color;\n"
    "vec3 linear_to_srgb(vec3 linear)\n"
    "{\n"
    "    bvec3 cutoff = lessThan(linear, vec3(0.0031308));\n"
    "    vec3 higher = vec3(1.055) * pow(linear, vec3(1.0 / 2.4)) - vec3(0.055);\n"
    "    vec3 lower = linear * vec3(12.92);\n"
    "    return mix(higher, lower, cutoff);\n"
    "}\n"
    "void main() {\n"
    "    vec3 color = texture(u_texture, vec3(v_uv, 0)).rgb;\n"
    "    color = linear_to_srgb(color);\n"
    "    frag_color = vec4(color, 1.0) * u_color;\n"
    "}\n";

char const *fs_grid_src =
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = vec4(0.2);\n"
    "}\n";

void renderer_init(Romantik_GameState *state)
{
    Walrus_Window *window = walrus_engine_window();
    i32 const      width  = walrus_window_width(window);
    i32 const      height = walrus_window_height(window);

    walrus_rhi_set_view_rect(0, 0, 0, width, height);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0xffd580ff, 1.0, 0);
    walrus_rhi_set_view_rect(1, width - width * 0.3 + 100, height - height * 0.3, width * 0.3, height * 0.3);
    walrus_rhi_set_view_clear(1, WR_RHI_CLEAR_DEPTH, 0, 1.0, 0);

    mat4 projection;
    glm_perspective(glm_rad(45.0), (float)width / height, 0.1, 1000.0, projection);
    walrus_rhi_set_view_transform(0, NULL, projection);
    walrus_rhi_set_view_transform(1, GLM_MAT4_IDENTITY, projection);

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
    walrus_vertex_layout_begin(&layout, 0);
    walrus_vertex_layout_add(&layout, 3, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_add(&layout, 2, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_end(&layout);
    state->layout = walrus_rhi_create_vertex_layout(&layout);

    walrus_vertex_layout_begin_instance(&layout, 1, 2);
    walrus_vertex_layout_add(&layout, 3, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_add(&layout, 1, WR_RHI_ATTR_FLOAT, false);
    walrus_vertex_layout_add(&layout, 1, WR_RHI_ATTR_MAT4, false);
    walrus_vertex_layout_end(&layout);
    state->ins_layout = walrus_rhi_create_vertex_layout(&layout);

    state->placed_buffer = walrus_rhi_create_buffer(NULL, 1000 * TERRAIN_COUNT * sizeof(HexInstanceBuffer), 0);
    state->avail_buffer  = walrus_rhi_create_buffer(NULL, 2000 * sizeof(HexInstanceBuffer), 0);

    HexInstanceBuffer queue_hexs[5];
    u32               queue_len = walrus_array_len(queue_hexs);
    for (u8 i = 0; i < queue_len; ++i) {
        mat4 model = GLM_MAT4_IDENTITY_INIT;
        glm_translate(model, (vec3){0, (i - queue_len / 2.0) * 0.2, -3.5});
        glm_rotate(model, glm_rad(30), (vec3){1, 0, 0});
        glm_mat4_copy(model, queue_hexs[i].model);
        romantik_get_terrain_color(i % TERRAIN_COUNT, queue_hexs[i].color);
        queue_hexs[i].border_type = 0;
    }
    state->queue_buffer = walrus_rhi_create_buffer(queue_hexs, queue_len * sizeof(HexInstanceBuffer), 0);

    state->u_texture = walrus_rhi_create_uniform("u_texture", WR_RHI_UNIFORM_SAMPLER, 1);
    state->u_color   = walrus_rhi_create_uniform("u_color", WR_RHI_UNIFORM_VEC4, 1);

    Walrus_ShaderHandle vs       = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, hex_src);
    Walrus_ShaderHandle vs_ins   = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, ins_hex_src);
    Walrus_ShaderHandle fs       = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_src);
    Walrus_ShaderHandle fs_layer = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, layer_fs_src);
    Walrus_ShaderHandle fs_grid  = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_grid_src);
    state->pick_shader           = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs, fs}, 2, true);
    state->map_shader            = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_ins, fs_layer}, 2, true);
    state->grid_shader           = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_ins, fs_grid}, 2, true);

    stbi_set_flip_vertically_on_load(true);
    u8 const  num_layers = 14;
    u32 const img_size   = 512 * 512 * 4;
    u8       *img_array  = walrus_malloc(num_layers * img_size);
    for (u8 i = 0; i < num_layers; ++i) {
        i32  x, y, c;
        char path[255];
        snprintf(path, 255, "imgs/%d.png", i);
        u8 *img = stbi_load(path, &x, &y, &c, 4);
        if (img == NULL) {
            walrus_error("fail to load image: %s", stbi_failure_reason());
        }
        else {
            memcpy(img_array + img_size * i, img, img_size);
            stbi_image_free(img);
        }
    }
    Walrus_TextureCreateInfo info;
    info.width       = 512;
    info.height      = 512;
    info.depth       = 1;
    info.num_mipmaps = 0;
    info.num_layers  = num_layers;
    info.format      = WR_RHI_FORMAT_RGBA8;
    info.ratio       = WR_RHI_RATIO_COUNT;
    info.flags       = WR_RHI_SAMPLER_MIN_LINEAR | WR_RHI_SAMPLER_MIP_LINEAR | WR_RHI_TEXTURE_SRGB;
    info.cube_map    = false;
    state->texture   = walrus_rhi_create_texture(&info, img_array);
    walrus_free(img_array);
}

void game_state_init(Romantik_GameState *state)
{
    camera_init(&state->cam);
    romantik_game_init(&state->game);
    renderer_init(state);

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
    HexInstanceBuffer *buffer = walrus_new(HexInstanceBuffer, game->num_avail_grids);
    romantik_compute_instance_buffer(&game->map, buffer, game->num_avail_grids, HEX_FLAG_AVAIL);
    walrus_rhi_update_buffer(state->avail_buffer, 0, game->num_avail_grids * sizeof(HexInstanceBuffer), buffer);
    walrus_free(buffer);

    state->hide_picker = true;

    glm_mat4_identity(state->model);
}

static void place_grid(Romantik_GameState *state, i32 q, i32 r)
{
    Romantik_Game *game = &state->game;
    HexMap        *map  = &game->map;

    if (romantik_place_grid(game, q, r)) {
        HexInstanceBuffer *buffer = walrus_new(HexInstanceBuffer, game->num_placed_grids);
        romantik_compute_instance_buffer(map, buffer, game->num_placed_grids, HEX_FLAG_PLACED);

        walrus_rhi_update_buffer(state->placed_buffer, 0, game->num_placed_grids * sizeof(HexInstanceBuffer), buffer);

        walrus_free(buffer);

        buffer = walrus_new(HexInstanceBuffer, game->num_avail_grids);
        romantik_compute_instance_buffer(map, buffer, game->num_avail_grids, HEX_FLAG_AVAIL);
        walrus_rhi_update_buffer(state->avail_buffer, 0, game->num_avail_grids * sizeof(HexInstanceBuffer), buffer);
        walrus_free(buffer);
    }
}

void game_state_tick(Romantik_GameState *state, f32 dt)
{
    camera_tick(&state->cam, dt);
    Walrus_Input  *input = walrus_engine_input();
    Romantik_Game *game  = &state->game;
    HexMap        *map   = &game->map;
    state->hide_picker   = true;
    if (walrus_input_pressed(input->keyboard, WR_KEY_D1)) {
        state->game.next_terrain = TERRAIN_GRASSLAND;
    }
    else if (walrus_input_pressed(input->keyboard, WR_KEY_D2)) {
        state->game.next_terrain = TERRAIN_WATER;
    }
    else if (walrus_input_pressed(input->keyboard, WR_KEY_D3)) {
        state->game.next_terrain = TERRAIN_SAND;
    }
    else if (walrus_input_pressed(input->keyboard, WR_KEY_D4)) {
        state->game.next_terrain = TERRAIN_SNOW;
    }
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
            if (romantik_test_flags(map, q, r, HEX_FLAG_AVAIL)) {
                romantik_compute_model(map, state->model, q, r);
                state->hide_picker = false;
                if (walrus_input_pressed(input->mouse, WR_MOUSE_BTN_LEFT)) {
                    place_grid(state, q, r);
                }
            }
        }
    }
}

void game_state_render(Romantik_GameState *state)
{
    CameraData *cam = &state->cam;

    walrus_rhi_set_view_transform(0, cam->view, NULL);

    walrus_rhi_set_vertex_buffer(0, state->buffer, state->layout, 0, UINT32_MAX);
    walrus_rhi_set_index_buffer(state->index_buffer, 0, UINT32_MAX);

    if (!state->hide_picker) {
        walrus_rhi_set_state(WR_RHI_STATE_DEFAULT | WR_RHI_STATE_BLEND_ALPHA, 0);
        walrus_rhi_set_transform(state->model);
        vec4 mul_color = {1.0, 1.0, 1.0, 0.7};
        vec4 color     = {0, 0, 0, 1.0};
        romantik_get_terrain_color(state->game.next_terrain, color);
        glm_vec4_mul(color, mul_color, color);
        walrus_rhi_set_uniform(state->u_color, 0, sizeof(vec4), color);
        walrus_rhi_submit(0, state->pick_shader, 0, WR_RHI_DISCARD_TRANSFORM | WR_RHI_DISCARD_STATE);
    }

    u32 unit = 0;
    walrus_rhi_set_texture(0, state->texture);
    walrus_rhi_set_uniform(state->u_texture, 0, sizeof(u32), &unit);
    if (state->game.num_placed_grids > 0) {
        walrus_rhi_set_instance_buffer(state->placed_buffer, state->ins_layout, 0, state->game.num_placed_grids);
        walrus_rhi_submit(0, state->map_shader, 0, WR_RHI_DISCARD_INSTANCE_DATA | WR_RHI_DISCARD_STATE);
    }

    walrus_rhi_set_state(WR_RHI_STATE_DEFAULT | WR_RHI_STATE_BLEND_ALPHA, 0);
    walrus_rhi_set_instance_buffer(state->avail_buffer, state->ins_layout, 0, state->game.num_avail_grids);
    walrus_rhi_submit(0, state->grid_shader, 0, WR_RHI_DISCARD_INSTANCE_DATA | WR_RHI_DISCARD_STATE);

    walrus_rhi_set_state(WR_RHI_STATE_DEFAULT | WR_RHI_STATE_BLEND_ALPHA, 0);
    walrus_rhi_set_instance_buffer(state->queue_buffer, state->ins_layout, 0, 10);
    walrus_rhi_submit(1, state->map_shader, 0, WR_RHI_DISCARD_ALL);
}
