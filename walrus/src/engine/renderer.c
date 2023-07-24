#include <engine/renderer.h>
#include <engine/shader_library.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/math.h>
#include <rhi/rhi.h>

#include <cglm/cglm.h>
#include <string.h>

static struct {
    vec2 pos;
    vec2 uv;
} quad_vertices[] = {{{-1.0, 1.0}, {0, 1}}, {{1.0, 1.0}, {1, 1}}, {{1.0, -1.0}, {1, 0}}, {{-1.0, -1.0}, {0, 0}}};

static u16 quad_indices[] = {0, 1, 2, 2, 3, 0};

typedef struct {
    Walrus_BufferHandle quad_vertices;
    Walrus_BufferHandle quad_indices;
    Walrus_LayoutHandle quad_layout;

    Walrus_UniformHandle u_has_morph;
    Walrus_UniformHandle u_morph_texture;

} RenderData;

static RenderData *s_data = NULL;

void walrus_renderer_init(void)
{
    s_data = walrus_new(RenderData, 1);

    Walrus_VertexLayout layout;
    walrus_vertex_layout_begin(&layout);
    walrus_vertex_layout_add(&layout, 0, 2, WR_RHI_COMPONENT_FLOAT, false);  // Pos
    walrus_vertex_layout_add(&layout, 1, 2, WR_RHI_COMPONENT_FLOAT, false);  // TexCoord
    walrus_vertex_layout_end(&layout);
    s_data->quad_layout = walrus_rhi_create_vertex_layout(&layout);

    s_data->quad_vertices = walrus_rhi_create_buffer(quad_vertices, sizeof(quad_vertices), 0);
    s_data->quad_indices  = walrus_rhi_create_buffer(quad_indices, sizeof(quad_indices), WR_RHI_BUFFER_INDEX);

    walrus_rhi_create_uniform("u_albedo", WR_RHI_UNIFORM_SAMPLER, 1);
    walrus_rhi_create_uniform("u_albedo_factor", WR_RHI_UNIFORM_VEC4, 1);
    walrus_rhi_create_uniform("u_emissive", WR_RHI_UNIFORM_SAMPLER, 1);
    walrus_rhi_create_uniform("u_emissive_factor", WR_RHI_UNIFORM_VEC3, 1);
    walrus_rhi_create_uniform("u_normal", WR_RHI_UNIFORM_SAMPLER, 1);
    walrus_rhi_create_uniform("u_normal_scale", WR_RHI_UNIFORM_FLOAT, 1);
    walrus_rhi_create_uniform("u_alpha_cutoff", WR_RHI_UNIFORM_FLOAT, 1);

    s_data->u_morph_texture = walrus_rhi_create_uniform("u_morph_texture", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_has_morph     = walrus_rhi_create_uniform("u_has_morph", WR_RHI_UNIFORM_BOOL, 1);
}

void walrus_renderer_shutdown(void)
{
    walrus_free(s_data);
}

static void setup_primitive(Walrus_MeshPrimitive const *prim)
{
    bool has_morph = prim->morph_target.id != WR_INVALID_HANDLE;
    walrus_rhi_set_uniform(s_data->u_has_morph, 0, sizeof(bool), &has_morph);
    if (has_morph) {
        u32 unit = walrus_rhi_get_caps()->max_texture_unit - 1;
        walrus_rhi_set_uniform(s_data->u_morph_texture, 0, sizeof(u32), &unit);
        walrus_rhi_set_texture(unit, prim->morph_target);
    }
    if (prim->indices.buffer.id != WR_INVALID_HANDLE) {
        if (prim->indices.index32) {
            walrus_rhi_set_index32_buffer(prim->indices.buffer, prim->indices.offset, prim->indices.num_indices);
        }
        else {
            walrus_rhi_set_index_buffer(prim->indices.buffer, prim->indices.offset, prim->indices.num_indices);
        }
    }
    for (u32 j = 0; j < prim->num_streams; ++j) {
        Walrus_PrimitiveStream const *stream = &prim->streams[j];
        walrus_rhi_set_vertex_buffer(j, stream->buffer, stream->layout_handle, stream->offset, stream->num_vertices);
    }
}

void walrus_renderer_submit_mesh(u16 view_id, Walrus_ProgramHandle shader, mat4 const world,
                                 Walrus_MeshPrimitive const *mesh)
{
    walrus_rhi_set_transform(world);

    setup_primitive(mesh);
    walrus_rhi_submit(view_id, shader, 0, WR_RHI_DISCARD_ALL);
}

void walrus_renderer_submit_quad(u16 view_id, Walrus_ProgramHandle shader)
{
    walrus_rhi_set_vertex_buffer(0, s_data->quad_vertices, s_data->quad_layout, 0, 4);
    walrus_rhi_set_index_buffer(s_data->quad_indices, 0, 6);
    walrus_rhi_submit(view_id, shader, 0, WR_RHI_DISCARD_ALL);
}
