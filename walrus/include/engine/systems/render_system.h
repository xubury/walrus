#pragma once

#include <engine/deferred_renderer.h>
#include <engine/renderer_mesh.h>
#include <flecs.h>

extern ECS_COMPONENT_DECLARE(Walrus_DeferredRenderer);
extern ECS_COMPONENT_DECLARE(Walrus_RenderMesh);
extern ECS_COMPONENT_DECLARE(Walrus_WeightResource);
extern ECS_COMPONENT_DECLARE(Walrus_SkinResource);

void walrus_render_system_init(void);

void walrus_render_system_shutdown(void);

void walrus_render_system_render(void);
