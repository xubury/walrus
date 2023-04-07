#include "rhi_p.h"

#include <core/macro.h>
#include <core/log.h>

static struct Walrus_GlContext {
    GLuint vao;
} s_gl;

static void submit(Walrus_RenderFrame *frame)
{
    u32 const resolution_height = frame->resolution.height;

    glBindVertexArray(s_gl.vao);

    for (i32 i = 0; i < frame->num_render_items; ++i) {
        Walrus_RenderItem const    *render_item = &frame->render_items[i];
        Walrus_RenderDraw const    *draw        = &render_item->draw;
        Walrus_RenderCompute const *compute     = &render_item->compute;
        u16 const                   view_id     = frame->view_ids[i];

        Walrus_RenderView const *view     = &frame->views[view_id];
        Walrus_Rect const       *viewport = &view->viewport;
        glViewport(viewport->x, resolution_height - viewport->height - viewport->y, viewport->width, viewport->height);

        walrus_unused(draw);
        walrus_unused(compute);
    }
}

void init_gl_backend(Walrus_RhiVTable *vtable)
{
    glGenVertexArrays(1, &s_gl.vao);
    glBindVertexArray(s_gl.vao);

    if (vtable) {
        vtable->submit_fn = submit;
    }
}
