#include <engine/batch_renderer.h>
#include <core/memory.h>
#include <core/log.h>
#include <core/math.h>
#include <rhi/rhi.h>

#include <string.h>
#include <cglm/cglm.h>

char const *vs_quad_src =
    "layout (location = 0) in vec2 a_pos;"
    "layout (location = 1) in vec2 a_texcoord;"
    "layout (location = 2) in vec2 a_uv0;"
    "layout (location = 3) in vec2 a_uv1;"
    "layout (location = 4) in float a_thickness;"
    "layout (location = 5) in float a_fade;"
    "layout (location = 6) in vec4 a_color;"
    "layout (location = 7) in vec4 a_border_color;"
    "layout (location = 8) in float a_tex_id;"
    "layout (location = 9) in mat4 a_model;"
    "uniform mat4 u_viewproj;"
    "out vec2 v_local_pos;"
    "out vec4 v_color;"
    "out float v_thickness;"
    "out float v_fade;"
    "out vec4 v_border_color;"
    "out vec2 v_texcoord;"
    "out float v_tex_id;"
    "void main()"
    "{"
    "    gl_Position = u_viewproj * a_model * vec4(a_pos, 0, 1.0);"
    "    v_local_pos = a_pos;"
    "    v_texcoord = a_texcoord * (a_uv1 - a_uv0) + a_uv0;"
    "    v_color = a_color;"
    "    v_thickness = a_thickness;"
    "    v_fade = a_fade;"
    "    v_border_color = a_border_color;"
    "    v_tex_id = a_tex_id;"
    "}";

char const *fs_quad =
    "out vec4 fragcolor;"
    "in vec2 v_local_pos;"
    "in vec4 v_color;"
    "in float v_thickness;"
    "in float v_fade;"
    "in vec4 v_border_color;"
    "in vec2 v_texcoord;"
    "in float v_tex_id;"
    "uniform sampler2D u_textures[16];"
    "void main()"
    "{"
    "    float boader = max(abs(v_local_pos.x), abs(v_local_pos.y)) - (0.5 - v_thickness / 2.f);"
    "    boader = smoothstep(0.0, v_fade, boader);"
    "    vec4 color = vec4(0);"
    "    switch(int(v_tex_id))"
    "	{"
    "		case  0: color = texture(u_textures[ 0], v_texcoord).rrrr; break;"
    "		case  1: color = texture(u_textures[ 1], v_texcoord); break;"
    "		case  2: color = texture(u_textures[ 2], v_texcoord); break;"
    "		case  3: color = texture(u_textures[ 3], v_texcoord); break;"
    "		case  4: color = texture(u_textures[ 4], v_texcoord); break;"
    "		case  5: color = texture(u_textures[ 5], v_texcoord); break;"
    "		case  6: color = texture(u_textures[ 6], v_texcoord); break;"
    "		case  7: color = texture(u_textures[ 7], v_texcoord); break;"
    "		case  8: color = texture(u_textures[ 8], v_texcoord); break;"
    "		case  9: color = texture(u_textures[ 9], v_texcoord); break;"
    "		case 10: color = texture(u_textures[10], v_texcoord); break;"
    "		case 11: color = texture(u_textures[11], v_texcoord); break;"
    "		case 12: color = texture(u_textures[12], v_texcoord); break;"
    "		case 13: color = texture(u_textures[13], v_texcoord); break;"
    "		case 14: color = texture(u_textures[14], v_texcoord); break;"
    "		case 15: color = texture(u_textures[15], v_texcoord); break;"
    "	}"
    "    fragcolor = mix(v_color, v_border_color, boader) * color;"
    "}";

char const *vs_circle_src =
    "layout (location = 0) in vec2 a_pos;"
    "layout (location = 1) in float a_thickness;"
    "layout (location = 2) in float a_fade;"
    "layout (location = 3) in vec4 a_color;"
    "layout (location = 4) in vec4 a_boarder_color;"
    "layout (location = 5) in mat4 a_model;"
    "out vec2 v_localpos;"
    "out float v_thickness;"
    "out float v_fade;"
    "out vec4 v_color;"
    "out vec4 v_boarder_color;"
    "uniform mat4 u_viewproj;"
    "void main()"
    "{"
    "    gl_Position = u_viewproj * a_model * vec4(a_pos, 0, 1.0);"
    "    v_localpos = a_pos;"
    "    v_thickness = a_thickness;"
    "    v_fade = a_fade;"
    "    v_color = a_color;"
    "    v_boarder_color = a_boarder_color;"
    "}";

char const *fs_circle_src =
    "out vec4 fragcolor;"
    "in vec2 v_localpos;"
    "in float v_thickness;"
    "in float v_fade;"
    "in vec4 v_color;"
    "in vec4 v_boarder_color;"
    "void main()"
    "{"
    "    float dist = 1 - length(v_localpos);"
    "    float circle = smoothstep(0.0, v_fade, dist);"
    "    if (circle == 0.0) {"
    "        discard;"
    "    }"
    "    float boarder = 1 - smoothstep(v_thickness, v_thickness + v_fade, dist);"
    "    fragcolor = mix(v_color, v_boarder_color, boarder);"
    "    fragcolor.a *= circle;"
    "}";

#define MAX_QUADS    65535
#define MAX_TEXTURES 16

static struct {
    vec2 pos;
    vec2 uv;
} quad_vertices[] = {{{-0.5, 0.5}, {0, 1}}, {{0.5, 0.5}, {1, 1}}, {{0.5, -0.5}, {1, 0}}, {{-0.5, -0.5}, {0, 0}}};

static struct {
    vec2 pos;
} circle_vertices[] = {{{-1.0, 1.0}}, {{1.0, 1.0}}, {{1.0, -1.0}}, {{-1.0, -1.0}}};

u16 quad_indices[] = {0, 1, 2, 2, 3, 0};

typedef struct {
    u16 view_id;
    u64 state;
    u8  discard;

    Walrus_ProgramHandle quad_shader;
    Walrus_ProgramHandle circle_shader;

    u32 num_quads;
    struct {
        vec2 uv0;
        vec2 uv1;
        f32  thickness;
        f32  fade;
        u32  color;
        u32  boarder_color;
        u8   tex_id;
        mat4 model;
    } quads[MAX_QUADS];

    u32 num_circles;
    struct {
        f32  thickness;
        f32  fade;
        u32  color;
        u32  boarder_color;
        mat4 model;
    } circles[MAX_QUADS];

    Walrus_LayoutHandle quad_layout;
    Walrus_LayoutHandle quad_ins_layout;
    Walrus_LayoutHandle circle_layout;
    Walrus_LayoutHandle circle_ins_layout;

    Walrus_UniformHandle u_textures;
    Walrus_TextureHandle white_texture;
    u32                  num_textures;
    Walrus_TextureHandle textures[MAX_TEXTURES];
} BatchRenderer;

static BatchRenderer *s_renderer = NULL;

void walrus_batch_render_init(void)
{
    s_renderer = walrus_new(BatchRenderer, 1);

    u8 data                   = 255;
    s_renderer->white_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_R8, 1, 0, &data, 1);
    s_renderer->u_textures    = walrus_rhi_create_uniform("u_textures", WR_RHI_UNIFORM_SAMPLER, 16);

    Walrus_ShaderHandle vs        = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, vs_quad_src);
    Walrus_ShaderHandle fs        = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_quad);
    s_renderer->quad_shader       = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs, fs}, 2, true);
    Walrus_ShaderHandle vs_circle = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, vs_circle_src);
    Walrus_ShaderHandle fs_circle = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_circle_src);
    s_renderer->circle_shader     = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_circle, fs_circle}, 2, true);
    walrus_rhi_destroy_shader(vs);
    walrus_rhi_destroy_shader(fs);
    walrus_rhi_destroy_shader(vs_circle);
    walrus_rhi_destroy_shader(fs_circle);

    Walrus_VertexLayout layout;
    walrus_vertex_layout_begin(&layout, 0);
    walrus_vertex_layout_add(&layout, 2, WR_RHI_ATTR_FLOAT, false);  // Pos
    walrus_vertex_layout_add(&layout, 2, WR_RHI_ATTR_FLOAT, false);  // TexCoord
    walrus_vertex_layout_end(&layout);
    s_renderer->quad_layout = walrus_rhi_create_vertex_layout(&layout);

    walrus_vertex_layout_begin_instance(&layout, 1, 2);
    walrus_vertex_layout_add(&layout, 2, WR_RHI_ATTR_FLOAT, false);  // uv0
    walrus_vertex_layout_add(&layout, 2, WR_RHI_ATTR_FLOAT, false);  // uv1
    walrus_vertex_layout_add(&layout, 1, WR_RHI_ATTR_FLOAT, false);  // Thickness
    walrus_vertex_layout_add(&layout, 1, WR_RHI_ATTR_FLOAT, false);  // Fade
    walrus_vertex_layout_add(&layout, 4, WR_RHI_ATTR_UINT8, true);   // Color
    walrus_vertex_layout_add(&layout, 4, WR_RHI_ATTR_UINT8, true);   // Boarder color
    walrus_vertex_layout_add(&layout, 1, WR_RHI_ATTR_UINT8, false);  // Texture Id
    walrus_vertex_layout_add(&layout, 1, WR_RHI_ATTR_MAT4, false);   // Model
    walrus_vertex_layout_end(&layout);
    s_renderer->quad_ins_layout = walrus_rhi_create_vertex_layout(&layout);

    walrus_vertex_layout_begin(&layout, 0);
    walrus_vertex_layout_add(&layout, 2, WR_RHI_ATTR_FLOAT, false);  // Pos
    walrus_vertex_layout_end(&layout);
    s_renderer->circle_layout = walrus_rhi_create_vertex_layout(&layout);

    walrus_vertex_layout_begin_instance(&layout, 1, 1);
    walrus_vertex_layout_add(&layout, 1, WR_RHI_ATTR_FLOAT, false);  // Thickness
    walrus_vertex_layout_add(&layout, 1, WR_RHI_ATTR_FLOAT, false);  // Fade
    walrus_vertex_layout_add(&layout, 4, WR_RHI_ATTR_UINT8, true);   // Color
    walrus_vertex_layout_add(&layout, 4, WR_RHI_ATTR_UINT8, true);   // Boarder color
    walrus_vertex_layout_add(&layout, 1, WR_RHI_ATTR_MAT4, false);   // Model
    walrus_vertex_layout_end(&layout);
    s_renderer->circle_ins_layout = walrus_rhi_create_vertex_layout(&layout);

    u32 textures[MAX_TEXTURES];
    for (u32 i = 0; i < MAX_TEXTURES; ++i) {
        textures[i] = i;
    }
    walrus_rhi_set_uniform(s_renderer->u_textures, 0, sizeof(textures), textures);
}

void walrus_batch_render_shutdown(void)
{
    walrus_rhi_destroy_program(s_renderer->quad_shader);
    walrus_free(s_renderer);
}

static void start_quads(void)
{
    s_renderer->num_quads    = 0;
    s_renderer->num_textures = 1;
}

static void start_circles(void)
{
    s_renderer->num_circles = 0;
}

void walrus_batch_render_begin(u16 view_id, u64 state)
{
    s_renderer->view_id = view_id;
    s_renderer->state   = state;
    s_renderer->discard = WR_RHI_DISCARD_ALL;
    start_quads();
    start_circles();
}

static void flush_quads(void)
{
    if (s_renderer->num_quads == 0) {
        return;
    }
    Walrus_TransientBuffer vertex_buffer;
    Walrus_TransientBuffer index_buffer;
    Walrus_TransientBuffer instance_buffer;
    u32 const              num_instances = s_renderer->num_quads;
    u32 const              vertex_size   = sizeof(quad_vertices[0]);
    u32 const              index_size    = sizeof(quad_indices[0]);
    u32 const              ins_size      = sizeof(s_renderer->quads[0]);

    bool const succ = walrus_rhi_alloc_transient_buffer(&instance_buffer, num_instances, ins_size) &&
                      walrus_rhi_alloc_transient_buffer(&vertex_buffer, 4, vertex_size) &&
                      walrus_rhi_alloc_transient_index_buffer(&index_buffer, 6, index_size);
    if (!succ) {
        walrus_error("Fail to allocated transient buffers");
        return;
    }
    for (u32 i = 0; i < s_renderer->num_textures; ++i) {
        walrus_rhi_set_texture(i, s_renderer->textures[i]);
    }
    memcpy(vertex_buffer.data, quad_vertices, 4 * vertex_size);
    memcpy(index_buffer.data, quad_indices, 6 * index_size);
    memcpy(instance_buffer.data, s_renderer->quads, num_instances * ins_size);
    walrus_rhi_set_transient_buffer(0, &vertex_buffer, s_renderer->quad_layout, 0, 4);
    walrus_rhi_set_transient_index_buffer(&index_buffer, 0, 6);
    walrus_rhi_set_transient_instance_buffer(&instance_buffer, s_renderer->quad_ins_layout, 0, num_instances);
    walrus_rhi_set_state(s_renderer->state, 0);
    walrus_rhi_submit(s_renderer->view_id, s_renderer->quad_shader, s_renderer->discard);
}

static void flush_circles(void)
{
    if (s_renderer->num_circles == 0) {
        return;
    }
    Walrus_TransientBuffer vertex_buffer;
    Walrus_TransientBuffer index_buffer;
    Walrus_TransientBuffer instance_buffer;
    u32 const              num_instances = s_renderer->num_circles;
    u32 const              vertex_size   = sizeof(circle_vertices[0]);
    u32 const              index_size    = sizeof(quad_indices[0]);
    u32 const              ins_size      = sizeof(s_renderer->circles[0]);

    bool const succ = walrus_rhi_alloc_transient_buffer(&instance_buffer, num_instances, ins_size) &&
                      walrus_rhi_alloc_transient_buffer(&vertex_buffer, 4, vertex_size) &&
                      walrus_rhi_alloc_transient_index_buffer(&index_buffer, 6, index_size);
    if (!succ) {
        walrus_error("Fail to allocated transient buffers");
        return;
    }

    memcpy(vertex_buffer.data, circle_vertices, 4 * vertex_size);
    memcpy(index_buffer.data, quad_indices, 6 * index_size);
    memcpy(instance_buffer.data, s_renderer->circles, num_instances * ins_size);
    walrus_rhi_set_transient_buffer(0, &vertex_buffer, s_renderer->circle_layout, 0, 4);
    walrus_rhi_set_transient_index_buffer(&index_buffer, 0, 6);
    walrus_rhi_set_transient_instance_buffer(&instance_buffer, s_renderer->circle_ins_layout, 0, num_instances);
    walrus_rhi_set_state(s_renderer->state, 0);
    walrus_rhi_submit(s_renderer->view_id, s_renderer->circle_shader, s_renderer->discard);
}

void warlus_batch_render_quad(vec3 pos, versor rot, vec2 size, u32 color, f32 thickness, u32 boarder_color, f32 fade)
{
    if (s_renderer->num_quads >= MAX_QUADS) {
        flush_quads();
        start_quads();
    }
    mat4 t = GLM_MAT4_IDENTITY_INIT;
    mat4 r;
    mat4 s = GLM_MAT4_IDENTITY_INIT;
    glm_translate(t, pos);
    glm_quat_mat4(rot, r);
    glm_scale(s, (vec3){size[0], size[1], 1});

    mat4 m;
    glm_mat4_mulN((mat4 *[]){&t, &r, &s}, 3, m);

    glm_mat4_copy(m, s_renderer->quads[s_renderer->num_quads].model);
    s_renderer->quads[s_renderer->num_quads].thickness     = thickness;
    s_renderer->quads[s_renderer->num_quads].fade          = fade;
    s_renderer->quads[s_renderer->num_quads].color         = color;
    s_renderer->quads[s_renderer->num_quads].boarder_color = boarder_color;
    s_renderer->quads[s_renderer->num_quads].tex_id        = 0;

    s_renderer->num_quads = walrus_u32satadd(s_renderer->num_quads, 1);
}

void warlus_batch_render_subtexture(Walrus_TextureHandle texture, vec2 uv0, vec2 uv1, vec3 pos, versor rot, vec2 size,
                                    u32 color, f32 thickness, u32 boarder_color, f32 fade)
{
    if (s_renderer->num_quads >= MAX_QUADS || s_renderer->num_textures >= MAX_TEXTURES) {
        flush_quads();
        start_quads();
    }
    mat4 t = GLM_MAT4_IDENTITY_INIT;
    mat4 r;
    mat4 s = GLM_MAT4_IDENTITY_INIT;
    glm_translate(t, pos);
    glm_quat_mat4(rot, r);
    glm_scale(s, (vec3){size[0], size[1], 1});

    mat4 m;
    glm_mat4_mulN((mat4 *[]){&t, &r, &s}, 3, m);

    glm_mat4_copy(m, s_renderer->quads[s_renderer->num_quads].model);
    glm_vec2_copy(uv0, s_renderer->quads[s_renderer->num_quads].uv0);
    glm_vec2_copy(uv1, s_renderer->quads[s_renderer->num_quads].uv1);
    s_renderer->quads[s_renderer->num_quads].thickness     = thickness;
    s_renderer->quads[s_renderer->num_quads].fade          = fade;
    s_renderer->quads[s_renderer->num_quads].color         = color;
    s_renderer->quads[s_renderer->num_quads].boarder_color = boarder_color;
    s_renderer->quads[s_renderer->num_quads].tex_id        = s_renderer->num_textures;

    s_renderer->textures[s_renderer->num_textures] = texture;

    s_renderer->num_quads    = walrus_u32satadd(s_renderer->num_quads, 1);
    s_renderer->num_textures = walrus_u32satadd(s_renderer->num_textures, 1);
}

void warlus_batch_render_texture(Walrus_TextureHandle texture, vec3 pos, versor rot, vec2 size, u32 color,
                                 f32 thickness, u32 boarder_color, f32 fade)
{
    warlus_batch_render_subtexture(texture, (vec2){0, 0}, (vec2){1, 1}, pos, rot, size, color, thickness, boarder_color,
                                   fade);
}

void warlus_batch_render_circle(vec3 pos, versor rot, f32 radius, u32 color, f32 thickness, u32 boarder_color, f32 fade)
{
    if (s_renderer->num_circles >= MAX_QUADS) {
        flush_circles();
        start_circles();
    }

    mat4 t = GLM_MAT4_IDENTITY_INIT;
    mat4 r;
    mat4 s = GLM_MAT4_IDENTITY_INIT;
    glm_translate(t, pos);
    glm_quat_mat4(rot, r);
    glm_scale(s, (vec3){radius, radius, 1});

    mat4 m;
    glm_mat4_mulN((mat4 *[]){&t, &r, &s}, 3, m);

    glm_mat4_copy(m, s_renderer->circles[s_renderer->num_circles].model);
    s_renderer->circles[s_renderer->num_circles].thickness     = thickness;
    s_renderer->circles[s_renderer->num_circles].fade          = fade;
    s_renderer->circles[s_renderer->num_circles].color         = color;
    s_renderer->circles[s_renderer->num_circles].boarder_color = boarder_color;

    s_renderer->num_circles = walrus_u32satadd(s_renderer->num_circles, 1);
}

void walrus_batch_render_end(void)
{
    flush_quads();
    flush_circles();
}
