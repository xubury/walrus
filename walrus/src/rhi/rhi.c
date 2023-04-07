#include "rhi_p.h"
#include <core/macro.h>

#include <math.h>
#include <string.h>

GLenum glew_init(void);

void wajs_setup_gl_context(void);

static Walrus_Rhi       s_rhi;
static Walrus_RhiVTable s_table;

Walrus_RhiError walrus_rhi_init(Walrus_RhiFlag flags)
{
    s_rhi.err_msg = "No error";
    s_rhi.flags   = flags;

    s_rhi.submit_frame.num_render_items = 0;
    s_rhi.submit_frame.num_views        = 0;

#if WR_PLATFORM != WR_PLATFORM_WASM
    GLenum err = glew_init();
    if (err != GLEW_OK) {
        s_rhi.err     = WR_RHI_INIT_GLEW_ERROR;
        s_rhi.err_msg = (char const *)glewGetErrorString(err);
    }
#else
    wajs_setup_gl_context();
#endif

    if (s_rhi.flags & WR_RHI_FLAG_OPENGL) {
        init_gl_backend(&s_table);
    }
    else {
        walrus_assert_msg(false, "No render backend specifed!");
    }
    return s_rhi.err;
}

char const *walrus_rhi_error_msg(void)
{
    return s_rhi.err_msg;
}

void walrus_rhi_set_resolution(i32 width, i32 height)
{
    s_rhi.submit_frame.resolution.width  = width;
    s_rhi.submit_frame.resolution.height = height;
}

void walrus_rhi_frame(void)
{
    Walrus_RenderFrame *frame = &s_rhi.submit_frame;
    memcpy(frame->views, s_rhi.views, sizeof(s_rhi.views));

    s_table.submit_fn(frame);
}

void walrus_rhi_submit(i16 view_id)
{
    Walrus_RenderFrame *frame = &s_rhi.submit_frame;

    u32 const render_item_idx = frame->num_render_items;
    frame->num_render_items   = fmin(WR_RHI_MAX_DRAW_CALLS, frame->num_render_items + 1);

    frame->view_ids[render_item_idx]          = view_id;
    frame->render_items[render_item_idx].draw = s_rhi.draw;
}

void walrus_rhi_set_view_rect(i16 view_id, i32 x, i32 y, i32 width, i32 height)
{
    width  = fmax(width, 1);
    height = fmax(height, 1);

    s_rhi.views[view_id].viewport.x      = x;
    s_rhi.views[view_id].viewport.y      = y;
    s_rhi.views[view_id].viewport.width  = width;
    s_rhi.views[view_id].viewport.height = height;
}
