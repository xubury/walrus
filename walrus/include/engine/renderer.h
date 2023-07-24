#pragma once

#include <core/type.h>
#include <core/transform.h>
#include <engine/camera.h>
#include <engine/model.h>
#include <engine/renderer_mesh.h>

typedef struct {
    Walrus_FramebufferHandle framebuffer;

    bool active;
    i16  x;
    i16  y;
    u16  width;
    u16  height;

} Walrus_Renderer;

void walrus_renderer_init(void);

void walrus_renderer_submit_mesh(u16 view_id, Walrus_ProgramHandle shader, mat4 const world,
                                 Walrus_MeshPrimitive const *mesh);

void walrus_renderer_submit_quad(u16 view_id, Walrus_ProgramHandle shader);
