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

typedef union {
    ImTextureID ptr;
    struct {
        Walrus_TextureHandle handle;
        u8                   flags;
        u8                   mip;
    } s;
} TextureDesc;

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
    ImFontAtlas_AddFontFromFileTTF(io->Fonts, "c:/windows/fonts/arialbd.ttf", 14, NULL, NULL);
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

    s_ctx->texture_shader = walrus_shader_library_load("imgui.shader");
    s_ctx->image_shader   = walrus_shader_library_load("imgui_image.shader");

    s_ctx->timestamp = walrus_sysclock(WR_SYS_CLOCK_UNIT_MICROSEC);
}

void walrus_imgui_shutdown(void)
{
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
    io->DeltaTime     = (f32)(ts - s_ctx->timestamp) / 1e6;
    s_ctx->timestamp  = ts;
    io->DisplaySize.x = width;
    io->DisplaySize.y = height;
    igNewFrame();
    ImGuizmo_BeginFrame();
}

static void render(ImDrawData *data)
{
    i32 const fb_width  = (i32)(data->DisplaySize.x * data->FramebufferScale.x);
    i32 const fb_height = (i32)(data->DisplaySize.y * data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }
    {
        f32 const x      = data->DisplayPos.x;
        f32 const y      = data->DisplayPos.y;
        f32 const width  = data->DisplaySize.x;
        f32 const height = data->DisplaySize.y;
        walrus_rhi_set_view_clear(s_ctx->view_id, WR_RHI_CLEAR_NONE, 0, 0, 0);
        walrus_rhi_set_view_rect(s_ctx->view_id, 0, 0, width, height);
        mat4 proj;
        glm_ortho(x, x + width, y + height, y, -1, 1, proj);
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

        bool succ = walrus_rhi_alloc_transient_buffer(&tvb, num_vertices, sizeof(ImDrawVert), sizeof(ImDrawVert)) &&
                    walrus_rhi_alloc_transient_index_buffer(&tib, num_indices, sizeof(ImDrawIdx));
        if (!succ) {
            walrus_error("Failed to allocate transient buffer (not enough space)");
            break;
        }
        memcpy(tvb.data, list->VtxBuffer.Data, num_vertices * sizeof(ImDrawVert));
        memcpy(tib.data, list->IdxBuffer.Data, num_indices * sizeof(ImDrawIdx));

        u32 offset = 0;

        u64 const alpha_flags = WR_RHI_STATE_BLEND_FUNC(WR_RHI_STATE_BLEND_SRC_ALPHA, WR_RHI_STATE_BLEND_INV_SRC_ALPHA);
        for (i32 j = 0; j < list->CmdBuffer.Size; ++j) {
            Walrus_ProgramHandle prog = s_ctx->texture_shader;
            Walrus_TextureHandle th   = s_ctx->font_atlas;
            ImDrawCmd           *cmd  = &list->CmdBuffer.Data[j];
            if (cmd->UserCallback) {
                cmd->UserCallback(list, cmd);
            }
            else if (cmd->ElemCount != 0) {
                u64 state = WR_RHI_STATE_WRITE_RGB | WR_RHI_STATE_WRITE_A;
                if (cmd->TextureId != NULL) {
                    TextureDesc tex = {cmd->TextureId};
                    if (IMGUI_TEXTURE_FLAGS_ALPHA_BLEND & tex.s.flags) {
                        state |= alpha_flags;
                    }

                    th = tex.s.handle;
                    if (tex.s.mip != 0) {
                        prog      = s_ctx->image_shader;
                        float mip = tex.s.mip;
                        walrus_rhi_set_uniform(s_ctx->u_lod, 0, sizeof(mip), &mip);
                    }
                }
                else {
                    state |= alpha_flags;
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
                    walrus_rhi_set_transient_vertex_buffer(0, &tvb, s_ctx->layout, 0, num_vertices);
                    walrus_rhi_set_transient_index_buffer(&tib, offset, cmd->ElemCount);
                    walrus_rhi_set_uniform(s_ctx->u_texture, 0, sizeof(u32), &(u32){0});
                    walrus_rhi_set_texture(0, th);
                    walrus_rhi_submit(s_ctx->view_id, prog, 0, WR_RHI_DISCARD_ALL);
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

static ImGuiMouseButton translate_mouse_btn(u8 btn)
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
static ImGuiKey translate_keyboard(Walrus_Keyboard key)
{
    switch (key) {
        case WR_KEY_TAB:
            return ImGuiKey_Tab;
        case WR_KEY_LEFT_ARROW:
            return ImGuiKey_LeftArrow;
        case WR_KEY_RIGHT_ARROW:
            return ImGuiKey_RightArrow;
        case WR_KEY_UP_ARROW:
            return ImGuiKey_UpArrow;
        case WR_KEY_DOWN_ARROW:
            return ImGuiKey_DownArrow;
        case WR_KEY_PAGE_UP:
            return ImGuiKey_PageUp;
        case WR_KEY_PAGE_DOWN:
            return ImGuiKey_PageDown;
        case WR_KEY_HOME:
            return ImGuiKey_Home;
        case WR_KEY_END:
            return ImGuiKey_End;
        case WR_KEY_INSERT:
            return ImGuiKey_Insert;
        case WR_KEY_DELETE:
            return ImGuiKey_Delete;
        case WR_KEY_BACKSPACE:
            return ImGuiKey_Backspace;
        case WR_KEY_SPACE:
            return ImGuiKey_Space;
        case WR_KEY_ENTER:
            return ImGuiKey_Enter;
        case WR_KEY_ESCAPE:
            return ImGuiKey_Escape;
        case WR_KEY_QUOTE:
            return ImGuiKey_Apostrophe;
        case WR_KEY_COMMA:
            return ImGuiKey_Comma;
        case WR_KEY_MINUS:
            return ImGuiKey_Minus;
        case WR_KEY_PERIOD:
            return ImGuiKey_Period;
        case WR_KEY_SLASH:
            return ImGuiKey_Slash;
        case WR_KEY_SEMICOLON:
            return ImGuiKey_Semicolon;
        case WR_KEY_EQUAL:
            return ImGuiKey_Equal;
        case WR_KEY_LEFT_BRACKET:
            return ImGuiKey_LeftBracket;
        case WR_KEY_BACK_SLASH:
            return ImGuiKey_Backslash;
        case WR_KEY_RIGHT_BRACKET:
            return ImGuiKey_RightBracket;
        case WR_KEY_BACK_QUOTE:
            return ImGuiKey_GraveAccent;
        case WR_KEY_CAPS_LOCK:
            return ImGuiKey_CapsLock;
        case WR_KEY_SCROLL_LOCK:
            return ImGuiKey_ScrollLock;
        case WR_KEY_NUM_LOCK:
            return ImGuiKey_NumLock;
        case WR_KEY_PRINT_SCREEN:
            return ImGuiKey_PrintScreen;
        case WR_KEY_PAUSE:
            return ImGuiKey_Pause;
        case WR_KEY_KEYPAD0:
            return ImGuiKey_Keypad0;
        case WR_KEY_KEYPAD1:
            return ImGuiKey_Keypad1;
        case WR_KEY_KEYPAD2:
            return ImGuiKey_Keypad2;
        case WR_KEY_KEYPAD3:
            return ImGuiKey_Keypad3;
        case WR_KEY_KEYPAD4:
            return ImGuiKey_Keypad4;
        case WR_KEY_KEYPAD5:
            return ImGuiKey_Keypad5;
        case WR_KEY_KEYPAD6:
            return ImGuiKey_Keypad6;
        case WR_KEY_KEYPAD7:
            return ImGuiKey_Keypad7;
        case WR_KEY_KEYPAD8:
            return ImGuiKey_Keypad8;
        case WR_KEY_KEYPAD9:
            return ImGuiKey_Keypad9;
        case WR_KEY_KEYPAD_DECIMAL:
            return ImGuiKey_KeypadDecimal;
        case WR_KEY_KEYPAD_DIVIDE:
            return ImGuiKey_KeypadDivide;
        case WR_KEY_KEYPAD_MULTIPLY:
            return ImGuiKey_KeypadMultiply;
        case WR_KEY_KEYPAD_SUBTRACT:
            return ImGuiKey_KeypadSubtract;
        case WR_KEY_KEYPAD_ADD:
            return ImGuiKey_KeypadAdd;
        case WR_KEY_KEYPAD_ENTER:
            return ImGuiKey_KeypadEnter;
        case WR_KEY_KEYPAD_EQUAL:
            return ImGuiKey_KeypadEqual;
        case WR_KEY_LEFT_SHIFT:
            return ImGuiKey_LeftShift;
        case WR_KEY_LEFT_CTRL:
            return ImGuiKey_LeftCtrl;
        case WR_KEY_LEFT_ALT:
            return ImGuiKey_LeftAlt;
        case WR_KEY_LEFT_SUPER:
            return ImGuiKey_LeftSuper;
        case WR_KEY_RIGHT_SHIFT:
            return ImGuiKey_RightShift;
        case WR_KEY_RIGHT_CTRL:
            return ImGuiKey_RightCtrl;
        case WR_KEY_RIGHT_ALT:
            return ImGuiKey_RightAlt;
        case WR_KEY_RIGHT_SUPER:
            return ImGuiKey_RightSuper;
        case WR_KEY_MENU:
            return ImGuiKey_Menu;
        case WR_KEY_D0:
            return ImGuiKey_0;
        case WR_KEY_D1:
            return ImGuiKey_1;
        case WR_KEY_D2:
            return ImGuiKey_2;
        case WR_KEY_D3:
            return ImGuiKey_3;
        case WR_KEY_D4:
            return ImGuiKey_4;
        case WR_KEY_D5:
            return ImGuiKey_5;
        case WR_KEY_D6:
            return ImGuiKey_6;
        case WR_KEY_D7:
            return ImGuiKey_7;
        case WR_KEY_D8:
            return ImGuiKey_8;
        case WR_KEY_D9:
            return ImGuiKey_9;
        case WR_KEY_A:
            return ImGuiKey_A;
        case WR_KEY_B:
            return ImGuiKey_B;
        case WR_KEY_C:
            return ImGuiKey_C;
        case WR_KEY_D:
            return ImGuiKey_D;
        case WR_KEY_E:
            return ImGuiKey_E;
        case WR_KEY_F:
            return ImGuiKey_F;
        case WR_KEY_G:
            return ImGuiKey_G;
        case WR_KEY_H:
            return ImGuiKey_H;
        case WR_KEY_I:
            return ImGuiKey_I;
        case WR_KEY_J:
            return ImGuiKey_J;
        case WR_KEY_K:
            return ImGuiKey_K;
        case WR_KEY_L:
            return ImGuiKey_L;
        case WR_KEY_M:
            return ImGuiKey_M;
        case WR_KEY_N:
            return ImGuiKey_N;
        case WR_KEY_O:
            return ImGuiKey_O;
        case WR_KEY_P:
            return ImGuiKey_P;
        case WR_KEY_Q:
            return ImGuiKey_Q;
        case WR_KEY_R:
            return ImGuiKey_R;
        case WR_KEY_S:
            return ImGuiKey_S;
        case WR_KEY_T:
            return ImGuiKey_T;
        case WR_KEY_U:
            return ImGuiKey_U;
        case WR_KEY_V:
            return ImGuiKey_V;
        case WR_KEY_W:
            return ImGuiKey_W;
        case WR_KEY_X:
            return ImGuiKey_X;
        case WR_KEY_Y:
            return ImGuiKey_Y;
        case WR_KEY_Z:
            return ImGuiKey_Z;
        case WR_KEY_F1:
            return ImGuiKey_F1;
        case WR_KEY_F2:
            return ImGuiKey_F2;
        case WR_KEY_F3:
            return ImGuiKey_F3;
        case WR_KEY_F4:
            return ImGuiKey_F4;
        case WR_KEY_F5:
            return ImGuiKey_F5;
        case WR_KEY_F6:
            return ImGuiKey_F6;
        case WR_KEY_F7:
            return ImGuiKey_F7;
        case WR_KEY_F8:
            return ImGuiKey_F8;
        case WR_KEY_F9:
            return ImGuiKey_F9;
        case WR_KEY_F10:
            return ImGuiKey_F10;
        case WR_KEY_F11:
            return ImGuiKey_F11;
        case WR_KEY_F12:
            return ImGuiKey_F12;
        default:
            return ImGuiKey_None;
    }
}

void walrus_imgui_process_event(Walrus_Event *event)
{
    ImGuiIO *io = igGetIO();
    switch (event->type) {
        case WR_EVENT_TYPE_TEXT:
            ImGuiIO_AddInputCharacter(io, event->text.unicode);
            break;
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
                ImGuiIO_AddMouseButtonEvent(io, translate_mouse_btn(event->button.button), event->button.state);
            }
            else if (event->button.device == WR_INPUT_KEYBOARD) {
                ImGuiIO_AddKeyEvent(io, translate_keyboard(event->button.button), event->button.state);
            }
            u8 mods      = event->button.mods;
            io->KeyShift = mods & WR_KEYMOD_SHIFT;
            io->KeyAlt   = mods & WR_KEYMOD_ALT;
            io->KeyCtrl  = mods & WR_KEYMOD_CTRL;
            io->KeySuper = mods & WR_KEYMOD_SUPER;
            break;
        default:
            break;
    }
}

ImTextureID igHandle(Walrus_TextureHandle handle, u8 flags, u8 mip)
{
    TextureDesc desc;
    desc.s.handle = handle;
    desc.s.flags  = flags;
    desc.s.mip    = mip;
    return desc.ptr;
}
