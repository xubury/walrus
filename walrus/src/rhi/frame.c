#include "frame.h"

void frame_init(Walrus_RenderFrame *frame)
{
    frame->num_render_items = 0;
    frame->num_views        = 0;
    frame->uniforms         = uniform_buffer_create(1 << 20);
}

void frame_shutdown(Walrus_RenderFrame *frame)
{
    uniform_buffer_destroy(frame->uniforms);
}

void frame_start(Walrus_RenderFrame *frame)
{
    frame->num_render_items = 0;
    uniform_buffer_start(frame->uniforms, 0);
}

void frame_finish(Walrus_RenderFrame *frame)
{
    uniform_buffer_finish(frame->uniforms);
}
