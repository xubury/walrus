#pragma once

#include <engine/imgui.h>
#include <engine/camera.h>

#include <flecs.h>

typedef struct {
    OPERATION op;
    MODE      mode;
} Walrus_TransformGuizmo;

typedef struct {
    char             name[256];
    bool             opened;
    ImGuiWindowFlags flags;
} Walrus_EditorWindow;

typedef void (*Walrus_ImGuiWidgetDrawFunc)(ecs_world_t *world, ecs_entity_t e);

typedef struct {
    Walrus_ImGuiWidgetDrawFunc func;
} Walrus_EditorWidget;

typedef struct {
    ecs_entity_t entity;
} Walrus_EntityObserver;

extern ECS_COMPONENT_DECLARE(Walrus_TransformGuizmo);
extern ECS_COMPONENT_DECLARE(Walrus_EditorWindow);
extern ECS_COMPONENT_DECLARE(Walrus_EditorWidget);
extern ECS_COMPONENT_DECLARE(Walrus_EntityObserver);
