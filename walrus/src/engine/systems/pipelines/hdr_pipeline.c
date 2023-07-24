#include <engine/systems/pipelines/hdr_pipeline.h>
#include <engine/renderer.h>
#include <engine/shader_library.h>
#include <core/log.h>
#include <core/macro.h>
#include <core/memory.h>
#include <rhi/rhi.h>

typedef struct {
    Walrus_UniformHandle u_color_buffer;
    Walrus_UniformHandle u_depth_buffer;

    Walrus_ProgramHandle copy_shader;
    Walrus_ProgramHandle hdr_shader;

    Walrus_FramebufferHandle hdr_buffer;
} HdrRenderData;

static HdrRenderData *s_data;

static void hdr_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    walrus_unused(node);

    u16 *view_id = walrus_fg_read_ptr(graph, "ViewSlot");

    Walrus_TextureHandle color_buffer = {walrus_fg_read(graph, "ColorBuffer")};

    walrus_rhi_set_view_rect_ratio(*view_id, WR_RHI_RATIO_EQUAL);
    walrus_rhi_set_view_clear(*view_id, WR_RHI_CLEAR_NONE, 0, 1.0, 0);
    walrus_rhi_set_framebuffer(*view_id, s_data->hdr_buffer);

    walrus_rhi_set_uniform(s_data->u_color_buffer, 0, sizeof(u32), &(u32){0});
    walrus_rhi_set_texture(0, color_buffer);

    walrus_rhi_set_state(WR_RHI_STATE_WRITE_RGB | WR_RHI_STATE_WRITE_A, 0);
    walrus_renderer_submit_quad(*view_id, s_data->hdr_shader);

    walrus_fg_write(graph, "ColorBuffer", walrus_rhi_get_texture(s_data->hdr_buffer, 0).id);

    ++(*view_id);
}

static void final_pass(Walrus_FrameGraph *graph, Walrus_FrameNode const *node)
{
    walrus_unused(node);

    u16 *view_id = walrus_fg_read_ptr(graph, "ViewSlot");

    Walrus_Renderer     *renderer     = walrus_fg_read_ptr(graph, "Renderer");
    Walrus_TextureHandle color_buffer = {walrus_fg_read(graph, "ColorBuffer")};
    Walrus_TextureHandle depth_buffer = {walrus_fg_read(graph, "DepthBuffer")};

    walrus_rhi_set_view_rect(*view_id, renderer->x, renderer->y, renderer->width, renderer->height);
    walrus_rhi_set_view_clear(*view_id, WR_RHI_CLEAR_NONE, 0, 1.0, 0);
    walrus_rhi_set_framebuffer(*view_id, renderer->framebuffer);

    walrus_rhi_set_uniform(s_data->u_color_buffer, 0, sizeof(u32), &(u32){0});
    walrus_rhi_set_texture(0, color_buffer);

    walrus_rhi_set_uniform(s_data->u_depth_buffer, 0, sizeof(u32), &(u32){1});
    walrus_rhi_set_texture(1, depth_buffer);

    walrus_rhi_set_state(WR_RHI_STATE_WRITE_RGB | WR_RHI_STATE_WRITE_A | WR_RHI_STATE_WRITE_Z, 0);
    walrus_renderer_submit_quad(*view_id, s_data->copy_shader);

    ++(*view_id);
}

Walrus_FramePipeline *walrus_hdr_pipeline_add(Walrus_FrameGraph *graph, char const *name)
{
    s_data = walrus_new(HdrRenderData, 1);

    Walrus_FramePipeline *pipeline = walrus_fg_add_pipeline(graph, name);
    walrus_fg_add_node(pipeline, hdr_pass, "HDR");
    walrus_fg_add_node(pipeline, final_pass, "Final");

    s_data->u_color_buffer = walrus_rhi_create_uniform("u_color_buffer", WR_RHI_UNIFORM_SAMPLER, 1);
    s_data->u_depth_buffer = walrus_rhi_create_uniform("u_depth_buffer", WR_RHI_UNIFORM_SAMPLER, 1);

    s_data->copy_shader = walrus_shader_library_load("copy.shader");
    s_data->hdr_shader  = walrus_shader_library_load("hdr.shader");

    Walrus_Attachment attachment = {0};
    attachment.handle            = walrus_rhi_create_texture(
        &(Walrus_TextureCreateInfo){
                       .ratio = WR_RHI_RATIO_EQUAL, .format = WR_RHI_FORMAT_RGB8, .num_mipmaps = 1, .flags = 0},
        NULL);
    attachment.access  = WR_RHI_ACCESS_WRITE;
    s_data->hdr_buffer = walrus_rhi_create_framebuffer(&attachment, 1);

    return pipeline;
}
