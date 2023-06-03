#include <engine/imgui.h>
#include <engine/input.h>
#include <engine/shader_library.h>
#include <core/log.h>
#include <core/sys.h>
#include <rhi/rhi.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/math.h>

#include <cglm/cglm.h>
#include <string.h>

typedef struct {
    ImGuiContext        *imgui;
    u16                  view_id;
    Walrus_TextureHandle font_atlas;
    Walrus_UniformHandle u_texture;
    Walrus_UniformHandle u_lod;
    Walrus_ProgramHandle texture_shader;
    Walrus_ProgramHandle image_shader;
    Walrus_LayoutHandle  layout;
    u64                  timestamp;
} Walrus_ImGuiContext;

static Walrus_ImGuiContext *s_ctx;

void *memalloc(size_t size, void *userdata)
{
    walrus_unused(userdata);
    return walrus_malloc(size);
}

void memrelease(void *ptr, void *userdata)
{
    walrus_unused(userdata);
    walrus_free(ptr);
}

void walrus_imgui_init(void)
{
    igSetAllocatorFunctions(memalloc, memrelease, NULL);
    s_ctx        = walrus_new(Walrus_ImGuiContext, 1);
    s_ctx->imgui = igCreateContext(NULL);

    ImGuiIO *io = igGetIO();
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io->ConfigWindowsMoveFromTitleBarOnly = true;

    io->DeltaTime     = 1.f / 60.f;
    io->DisplaySize.x = 1024;
    io->DisplaySize.y = 768;

    u8 *data;
    i32 width;
    i32 height;
    ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &data, &width, &height, NULL);
    s_ctx->font_atlas =
        walrus_rhi_create_texture2d(width, height, WR_RHI_FORMAT_RGBA8, 0, 0, walrus_memdup(data, width * height * 4));
    s_ctx->u_texture = walrus_rhi_create_uniform("u_texture", WR_RHI_UNIFORM_SAMPLER, 1);
    s_ctx->u_lod     = walrus_rhi_create_uniform("u_lod", WR_RHI_UNIFORM_FLOAT, 1);

    Walrus_VertexLayout layout;
    walrus_vertex_layout_begin(&layout);
    walrus_vertex_layout_add(&layout, 0, 2, WR_RHI_COMPONENT_FLOAT, false);  // pos
    walrus_vertex_layout_add(&layout, 1, 2, WR_RHI_COMPONENT_FLOAT, false);  // uv
    walrus_vertex_layout_add(&layout, 2, 4, WR_RHI_COMPONENT_UINT8, true);   // color
    walrus_vertex_layout_end(&layout);

    s_ctx->layout = walrus_rhi_create_vertex_layout(&layout);

    s_ctx->texture_shader = walrus_rhi_create_program(
        (Walrus_ShaderHandle[]){walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_imgui.glsl"),
                                walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_imgui.glsl")},
        2, true);
    s_ctx->image_shader = walrus_rhi_create_program(
        (Walrus_ShaderHandle[]){walrus_shader_library_load(WR_RHI_SHADER_VERTEX, "vs_imgui.glsl"),
                                walrus_shader_library_load(WR_RHI_SHADER_FRAGMENT, "fs_imgui_lod.glsl")},
        2, true);

    s_ctx->timestamp = walrus_sysclock(WR_SYS_CLOCK_UNIT_MICROSEC);
}

void walrus_imgui_shutdown(void)
{
    walrus_rhi_destroy_program(s_ctx->texture_shader);
    walrus_rhi_destroy_program(s_ctx->image_shader);
    walrus_rhi_destroy_vertex_layout(s_ctx->layout);
    walrus_rhi_destroy_uniform(s_ctx->u_lod);
    walrus_rhi_destroy_uniform(s_ctx->u_texture);
    walrus_rhi_destroy_texture(s_ctx->font_atlas);
    igDestroyContext(s_ctx->imgui);
    walrus_free(s_ctx);
    s_ctx = NULL;
}

void walrus_imgui_new_frame(u32 width, u32 height, u16 view_id)
{
    s_ctx->view_id    = view_id;
    ImGuiIO *io       = igGetIO();
    u64      ts       = walrus_sysclock(WR_SYS_CLOCK_UNIT_MICROSEC);
    io->DeltaTime     = walrus_max((f32)(ts - s_ctx->timestamp) / 1e6, 1.0 / 1000.f);
    s_ctx->timestamp  = ts;
    io->DisplaySize.x = width;
    io->DisplaySize.y = height;
    igNewFrame();
}

static void render(ImDrawData *data)
{
    i32 const fb_width  = (int)(data->DisplaySize.x * data->FramebufferScale.x);
    i32 const fb_height = (int)(data->DisplaySize.y * data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }
    {
        f32 const x      = data->DisplayPos.x;
        f32 const y      = data->DisplayPos.y;
        f32 const width  = data->DisplaySize.x;
        f32 const height = data->DisplaySize.y;
        walrus_rhi_set_view_clear(s_ctx->view_id, WR_RHI_CLEAR_COLOR, 0, 0, 0);
        walrus_rhi_set_view_rect(s_ctx->view_id, 0, 0, width, height);
        mat4 proj;
        glm_ortho(x, x + width, y + height, y, 0, 1000, proj);
        walrus_rhi_set_view_transform(s_ctx->view_id, GLM_MAT4_IDENTITY, proj);
        walrus_rhi_set_view_mode(s_ctx->view_id, WR_RHI_VIEWMODE_SEQUENTIAL);
    }

    ImVec2 const clip_pos   = data->DisplayPos;
    ImVec2 const clip_scale = data->FramebufferScale;
    for (i32 i = 0; i < data->CmdListsCount; ++i) {
        ImDrawList *list = data->CmdLists[i];

        Walrus_TransientBuffer tvb;
        Walrus_TransientBuffer tib;
        u32 const              num_vertices = list->VtxBuffer.Size;
        u32 const              num_indices  = list->IdxBuffer.Size;

        bool succ = walrus_rhi_alloc_transient_buffer(&tvb, num_vertices, sizeof(ImDrawVert)) &&
                    walrus_rhi_alloc_transient_index_buffer(&tib, num_indices, sizeof(ImDrawIdx));
        if (!succ) {
            walrus_error("Failed to allocate transient buffer (not enough space)");
            break;
        }
        memcpy(tvb.data, list->VtxBuffer.Data, num_vertices * sizeof(ImDrawVert));
        memcpy(tib.data, list->IdxBuffer.Data, num_indices * sizeof(ImDrawIdx));

        u32 offset = 0;

        Walrus_TextureHandle th = s_ctx->font_atlas;
        for (i32 j = 0; j < list->CmdBuffer.Size; ++j) {
            ImDrawCmd *cmd = &list->CmdBuffer.Data[j];
            if (cmd->UserCallback) {
                cmd->UserCallback(list, cmd);
            }
            else if (cmd->ElemCount != 0) {
                u64 state = WR_RHI_STATE_WRITE_RGB | WR_RHI_STATE_WRITE_A;
                if (cmd->TextureId != NULL) {
                }
                else {
                    state |= WR_RHI_STATE_BLEND_FUNC(WR_RHI_STATE_BLEND_SRC_ALPHA, WR_RHI_STATE_BLEND_INV_SRC_ALPHA);
                }
                ImVec4 clip_rect;
                clip_rect.x = (cmd->ClipRect.x - clip_pos.x) * clip_scale.x;
                clip_rect.y = (cmd->ClipRect.y - clip_pos.y) * clip_scale.y;
                clip_rect.z = (cmd->ClipRect.z - clip_pos.x) * clip_scale.x;
                clip_rect.w = (cmd->ClipRect.w - clip_pos.y) * clip_scale.y;
                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.f && clip_rect.w >= 0.f) {
                    u16 const x      = walrus_max(clip_rect.x, 0.0f);
                    u16 const y      = walrus_max(clip_rect.y, 0.0f);
                    u16 const width  = walrus_min(clip_rect.z, 65535.f);
                    u16 const height = walrus_min(clip_rect.w, 65535.f);
                    walrus_rhi_set_scissor(x, y, width - x, height - y);
                    walrus_rhi_set_state(state, 0);
                    walrus_rhi_set_transient_buffer(0, &tvb, s_ctx->layout, 0, num_vertices);
                    walrus_rhi_set_transient_index_buffer(&tib, offset, cmd->ElemCount);
                    u32 unit = 0;
                    walrus_rhi_set_uniform(s_ctx->u_texture, 0, sizeof(u32), &unit);
                    walrus_rhi_set_texture(unit, th);
                    walrus_rhi_submit(s_ctx->view_id, s_ctx->texture_shader, 0, WR_RHI_DISCARD_ALL);
                }
            }
            offset += cmd->ElemCount * sizeof(ImDrawIdx);
        }
    }
}

void walrus_imgui_end_frame(void)
{
    igRender();
    render(igGetDrawData());
}

static ImGuiMouseButton convert_mouse_btn(uint8_t btn)
{
    switch (btn) {
        case WR_MOUSE_BTN_LEFT:
            return ImGuiMouseButton_Left;
        case WR_MOUSE_BTN_MIDDLE:
            return ImGuiMouseButton_Middle;
        case WR_MOUSE_BTN_RIGHT:
            return ImGuiMouseButton_Right;
        default:
            return 0;
    }
}

void walrus_imgui_process_event(Walrus_Event *event)
{
    ImGuiIO *io = igGetIO();
    switch (event->type) {
        case WR_EVENT_TYPE_AXIS:
            if (event->axis.device == WR_INPUT_MOUSE) {
                if (event->axis.axis == WR_MOUSE_AXIS_CURSOR) {
                    ImGuiIO_AddMousePosEvent(io, event->axis.x, event->axis.y);
                }
                else if (event->axis.axis == WR_MOUSE_AXIS_WHEEL) {
                    ImGuiIO_AddMouseWheelEvent(io, event->axis.x, event->axis.y);
                }
            }
            break;
        case WR_EVENT_TYPE_BUTTON:
            if (event->button.device == WR_INPUT_MOUSE) {
                ImGuiIO_AddMouseButtonEvent(io, convert_mouse_btn(event->button.button), event->button.state);
            }
            break;
        default:
            break;
    }
}
