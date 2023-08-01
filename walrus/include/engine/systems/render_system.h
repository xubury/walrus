#pragma once

#include <engine/renderer_mesh.h>
#include <engine/frame_graph.h>
#include <engine/system.h>
#include <flecs.h>

extern ECS_COMPONENT_DECLARE(Walrus_RenderMesh);
extern ECS_COMPONENT_DECLARE(Walrus_Material);
extern ECS_COMPONENT_DECLARE(Walrus_WeightResource);
extern ECS_COMPONENT_DECLARE(Walrus_SkinResource);

typedef struct {
    Walrus_FrameGraph        render_graph;
    Walrus_FramebufferHandle backrt;
    Walrus_Material          default_material;
} RenderSystem;

POLY_DECLARE_DERIVED(Walrus_System, RenderSystem, render_system_create)
