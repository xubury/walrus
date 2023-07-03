#include <engine/batch_renderer.h>
#include <engine/shader_library.h>
#include <core/memory.h>
#include <core/log.h>
#include <core/math.h>
#include <rhi/rhi.h>

#include <string.h>
#include <cglm/cglm.h>

#define LITTLE_ENDIAN_COLOR(color) \
    (((color)&0xff000000) >> 24 | ((color)&0x00ff0000) >> 8 | ((color)&0x0000ff00) << 8 | ((color)&0xff) << 24)

#define MAX_QUADS    65535
#define MAX_TEXTURES 16

static struct {
    vec2 pos;
    vec2 uv;
} quad_vertices[] = {{{-0.5, 0.5}, {0, 1}}, {{0.5, 0.5}, {1, 1}}, {{0.5, -0.5}, {1, 0}}, {{-0.5, -0.5}, {0, 0}}};

static struct {
    vec2 pos;
} circle_vertices[] = {{{-1.0, 1.0}}, {{1.0, 1.0}}, {{1.0, -1.0}}, {{-1.0, -1.0}}};

static u16 quad_indices[] = {0, 1, 2, 2, 3, 0};

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

    u32 data                  = 0xffffffff;
    s_renderer->white_texture = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 1, 0, &data);
    s_renderer->u_textures    = walrus_rhi_create_uniform("u_textures", WR_RHI_UNIFORM_SAMPLER, 16);

    Walrus_ShaderHandle vs_quad   = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_quad_batch.glsl");
    Walrus_ShaderHandle fs_quad   = walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_quad_batch.glsl");
    s_renderer->quad_shader       = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_quad, fs_quad}, 2, true);
    Walrus_ShaderHandle vs_circle = walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_circle_batch.glsl");
    Walrus_ShaderHandle fs_circle = walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_circle_batch.glsl");
    s_renderer->circle_shader     = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs_circle, fs_circle}, 2, true);

    Walrus_VertexLayout layout;
    walrus_vertex_layout_begin(&layout);
    walrus_vertex_layout_add(&layout, 0, 2, WR_RHI_COMPONENT_FLOAT, false);  // Pos
    walrus_vertex_layout_add(&layout, 1, 2, WR_RHI_COMPONENT_FLOAT, false);  // TexCoord
    walrus_vertex_layout_end(&layout);
    s_renderer->quad_layout = walrus_rhi_create_vertex_layout(&layout);

    walrus_vertex_layout_begin_instance(&layout, 1);
    walrus_vertex_layout_add(&layout, 2, 2, WR_RHI_COMPONENT_FLOAT, false);  // uv0
    walrus_vertex_layout_add(&layout, 3, 2, WR_RHI_COMPONENT_FLOAT, false);  // uv1
    walrus_vertex_layout_add(&layout, 4, 1, WR_RHI_COMPONENT_FLOAT, false);  // Thickness
    walrus_vertex_layout_add(&layout, 5, 1, WR_RHI_COMPONENT_FLOAT, false);  // Fade
    walrus_vertex_layout_add(&layout, 6, 4, WR_RHI_COMPONENT_UINT8, true);   // Color
    walrus_vertex_layout_add(&layout, 7, 4, WR_RHI_COMPONENT_UINT8, true);   // Boarder color
    walrus_vertex_layout_add(&layout, 8, 1, WR_RHI_COMPONENT_UINT8, false);  // Texture Id
    walrus_vertex_layout_add_mat4(&layout, 9);                               // Model
    walrus_vertex_layout_end(&layout);
    s_renderer->quad_ins_layout = walrus_rhi_create_vertex_layout(&layout);

    walrus_vertex_layout_begin(&layout);
    walrus_vertex_layout_add(&layout, 0, 2, WR_RHI_COMPONENT_FLOAT, false);  // Pos
    walrus_vertex_layout_end(&layout);
    s_renderer->circle_layout = walrus_rhi_create_vertex_layout(&layout);

    walrus_vertex_layout_begin_instance(&layout, 1);
    walrus_vertex_layout_add(&layout, 1, 1, WR_RHI_COMPONENT_FLOAT, false);  // Thickness
    walrus_vertex_layout_add(&layout, 2, 1, WR_RHI_COMPONENT_FLOAT, false);  // Fade
    walrus_vertex_layout_add(&layout, 3, 4, WR_RHI_COMPONENT_UINT8, true);   // Color
    walrus_vertex_layout_add(&layout, 4, 4, WR_RHI_COMPONENT_UINT8, true);   // Boarder color
    walrus_vertex_layout_add_mat4(&layout, 5);                               // Model
    walrus_vertex_layout_end(&layout);
    s_renderer->circle_ins_layout = walrus_rhi_create_vertex_layout(&layout);

    u32 textures[MAX_TEXTURES];
    for (u32 i = 0; i < MAX_TEXTURES; ++i) {
        textures[i]                = i;
        s_renderer->textures[i].id = WR_INVALID_HANDLE;
    }
    s_renderer->textures[0] = s_renderer->white_texture;

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

    bool const succ = walrus_rhi_alloc_transient_buffer(&instance_buffer, num_instances, ins_size,
                                                        walrus_rhi_get_caps()->instance_align) &&
                      walrus_rhi_alloc_transient_buffer(&vertex_buffer, 4, vertex_size, vertex_size) &&
                      walrus_rhi_alloc_transient_index_buffer(&index_buffer, 6, index_size);
    if (!succ) {
        walrus_error("Fail to allocated transient buffers");
        return;
    }
    u32 *textures = walrus_alloca(s_renderer->num_textures * sizeof(u32));
    for (u32 i = 0; i < s_renderer->num_textures; ++i) {
        textures[i] = i;
        walrus_rhi_set_texture(i, s_renderer->textures[i]);
    }
    walrus_rhi_set_uniform(s_renderer->u_textures, 0, sizeof(u32) * s_renderer->num_textures, textures);

    memcpy(vertex_buffer.data, quad_vertices, 4 * vertex_size);
    memcpy(index_buffer.data, quad_indices, 6 * index_size);
    memcpy(instance_buffer.data, s_renderer->quads, num_instances * ins_size);
    walrus_rhi_set_transient_vertex_buffer(0, &vertex_buffer, s_renderer->quad_layout, 0, 4);
    walrus_rhi_set_transient_index_buffer(&index_buffer, 0, 6);
    walrus_rhi_set_transient_instance_buffer(&instance_buffer, s_renderer->quad_ins_layout, 0, num_instances);
    walrus_rhi_set_state(s_renderer->state, 0);
    walrus_rhi_submit(s_renderer->view_id, s_renderer->quad_shader, 0, s_renderer->discard);
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

    bool const succ = walrus_rhi_alloc_transient_buffer(&instance_buffer, num_instances, ins_size,
                                                        walrus_rhi_get_caps()->instance_align) &&
                      walrus_rhi_alloc_transient_buffer(&vertex_buffer, 4, vertex_size, vertex_size) &&
                      walrus_rhi_alloc_transient_index_buffer(&index_buffer, 6, index_size);
    if (!succ) {
        walrus_error("Fail to allocated transient buffers");
        return;
    }

    memcpy(vertex_buffer.data, circle_vertices, 4 * vertex_size);
    memcpy(index_buffer.data, quad_indices, 6 * index_size);
    memcpy(instance_buffer.data, s_renderer->circles, num_instances * ins_size);
    walrus_rhi_set_transient_vertex_buffer(0, &vertex_buffer, s_renderer->circle_layout, 0, 4);
    walrus_rhi_set_transient_index_buffer(&index_buffer, 0, 6);
    walrus_rhi_set_transient_instance_buffer(&instance_buffer, s_renderer->circle_ins_layout, 0, num_instances);
    walrus_rhi_set_state(s_renderer->state, 0);
    walrus_rhi_submit(s_renderer->view_id, s_renderer->circle_shader, 0, s_renderer->discard);
}

void walrus_batch_render_quad(vec3 pos, versor rot, vec2 size, u32 color, f32 thickness, u32 boarder_color, f32 fade)
{
    if (s_renderer->num_quads >= MAX_QUADS) {
        flush_quads();
        start_quads();
    }

    color         = LITTLE_ENDIAN_COLOR(color);
    boarder_color = LITTLE_ENDIAN_COLOR(boarder_color);

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

    ++s_renderer->num_quads;
}

void walrus_batch_render_subtexture(Walrus_TextureHandle texture, vec2 uv0, vec2 uv1, vec3 pos, versor rot, vec2 size,
                                    u32 color, f32 thickness, u32 boarder_color, f32 fade)
{
    if (s_renderer->num_quads >= MAX_QUADS || s_renderer->num_textures >= MAX_TEXTURES) {
        flush_quads();
        start_quads();
    }

    color         = LITTLE_ENDIAN_COLOR(color);
    boarder_color = LITTLE_ENDIAN_COLOR(boarder_color);

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

    u32 tex_id = s_renderer->num_textures;
    for (i32 i = s_renderer->num_textures - 1; i > 1; --i) {
        if (s_renderer->textures[i].id == texture.id) {
            tex_id = i;
            break;
        }
    }

    s_renderer->quads[s_renderer->num_quads].tex_id = tex_id;

    if (tex_id == s_renderer->num_textures) {
        s_renderer->textures[s_renderer->num_textures] = texture;
        ++s_renderer->num_textures;
    }

    ++s_renderer->num_quads;
}

void walrus_batch_render_texture(Walrus_TextureHandle texture, vec3 pos, versor rot, vec2 size, u32 color,
                                 f32 thickness, u32 boarder_color, f32 fade)
{
    walrus_batch_render_subtexture(texture, (vec2){0, 0}, (vec2){1, 1}, pos, rot, size, color, thickness, boarder_color,
                                   fade);
}

void walrus_batch_render_string(Walrus_FontTexture *font, char const *str, vec3 pos, versor rot, vec2 size, u32 color)
{
    vec3 cur;
    glm_vec3_copy(pos, cur);
    for (u32 i = 0; str[i] != 0; ++i) {
        if (str[i] == '\n') {
            cur[0] = pos[0];
            cur[1] += font->font_height;
            continue;
        }
        Walrus_GlyphMetrics metrics;
        if (walrus_font_texture_unicode_metrics(font, str[i], &metrics)) {
            vec3 font_pos;
            vec2 font_size;
            vec2 offset0;

            glm_vec2_copy(cur, font_pos);
            glm_vec2_sub(metrics.offset1, metrics.offset0, font_size);
            glm_vec2_mul(font_size, size, font_size);
            font_pos[0] += font_size[0] / 2.f;
            font_pos[1] += font_size[1] / 2.f;
            glm_vec2_mul(metrics.offset0, size, offset0);
            glm_vec2_add(font_pos, offset0, font_pos);

            vec2 uv0, uv1;
            walrus_font_texture_unicode_uv(font, str[i], uv0, uv1);
            walrus_batch_render_subtexture(font->handle, uv0, uv1, font_pos, rot, font_size, color, 0, 0, 0);
            cur[0] += metrics.advance * size[0];
        }
    }
}

void walrus_batch_render_circle(vec3 pos, versor rot, f32 radius, u32 color, f32 thickness, u32 boarder_color, f32 fade)
{
    if (s_renderer->num_circles >= MAX_QUADS) {
        flush_circles();
        start_circles();
    }

    color         = LITTLE_ENDIAN_COLOR(color);
    boarder_color = LITTLE_ENDIAN_COLOR(boarder_color);

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

    ++s_renderer->num_circles;
}

void walrus_batch_render_end(void)
{
    flush_quads();
    flush_circles();
}
