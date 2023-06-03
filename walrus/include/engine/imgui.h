#pragma once

#include <core/type.h>
#include <engine/event.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

void walrus_imgui_init(void);

void walrus_imgui_shutdown(void);

void walrus_imgui_new_frame(u32 width, u32 height, u16 view_id);

void walrus_imgui_end_frame(void);

void walrus_imgui_process_event(Walrus_Event *event);
