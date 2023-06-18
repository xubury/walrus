#pragma once

#include <core/type.h>
#include <engine/event.h>
#include <rhi/type.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimguizmo.h>
#include <cimgui.h>

void walrus_imgui_init(void);

void walrus_imgui_shutdown(void);

void walrus_imgui_new_frame(u32 width, u32 height, u16 view_id);

void walrus_imgui_end_frame(void);

void walrus_imgui_process_event(Walrus_Event* event);

#define IMGUI_TEXTURE_FLAGS_NONE        UINT8_C(0x00)
#define IMGUI_TEXTURE_FLAGS_ALPHA_BLEND UINT8_C(0x01)
ImTextureID igHandle(Walrus_TextureHandle handle, u8 flags, u8 mip);
