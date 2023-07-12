#pragma once

#include <engine/imgui.h>
#include <engine/camera.h>

#include <flecs.h>

typedef struct {
    OPERATION op;
    MODE      mode;
} Walrus_TransformGuizmo;

typedef struct {
    u32 width;
    u32 height;
} Walrus_EditorRenderer;

extern ECS_COMPONENT_DECLARE(Walrus_TransformGuizmo);
extern ECS_COMPONENT_DECLARE(Walrus_EditorRenderer);

void walrus_editor_system_init(void);
void walrus_editor_system_render(void);
