#pragma once

#include <engine/imgui.h>
#include <engine/camera.h>

#include <flecs.h>

typedef struct {
    OPERATION op;
    MODE      mode;
} Walrus_TransformGuizmo;

typedef struct {
    char name[256];
    bool opened;
    ImGuiWindowFlags flags;
} Walrus_EditorWindow;

typedef void (*Walrus_ImGuiWidgetDrawFunc)(void);

typedef struct {
    Walrus_ImGuiWidgetDrawFunc func;
} Walrus_EditorWidget;

extern ECS_COMPONENT_DECLARE(Walrus_TransformGuizmo);
extern ECS_COMPONENT_DECLARE(Walrus_EditorWindow);
extern ECS_COMPONENT_DECLARE(Walrus_EditorWidget);

void walrus_editor_system_init(void);
void walrus_editor_system_render(void);
