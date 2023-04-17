#include <core/macro.h>
#include <core/rect.h>
#include <core/math.h>
#include <core/memory.h>
#include <engine/engine.h>
#include <rhi/rhi.h>

#include "game_state.h"

void on_render(Walrus_App *app)
{
    Romantik_GameState *state = walrus_app_userdata(app);
    CameraData         *cam   = &state->cam;

    walrus_rhi_set_state(WR_RHI_STATE_DEFAULT | WR_RHI_STATE_BLEND_ALPHA, 0);
    walrus_rhi_set_view_transform(0, cam->view, NULL);

    walrus_rhi_set_vertex_buffer(0, state->buffer, state->layout, 0, UINT32_MAX);
    walrus_rhi_set_index_buffer(state->index_buffer, 0, UINT32_MAX);

    if (!state->hide_picker) {
        walrus_rhi_set_transform(state->model);
        walrus_rhi_submit(0, state->pick_shader, WR_RHI_DISCARD_TRANSFORM);
    }

    walrus_rhi_set_texture(0, state->u_texture, state->texture);
    walrus_rhi_set_instance_buffer(state->placed_buffer, state->model_layout, 0, state->game.num_placed_grids);
    walrus_rhi_submit(0, state->map_shader, WR_RHI_DISCARD_INSTANCE_DATA);

    walrus_rhi_set_instance_buffer(state->avail_buffer, state->model_layout, 0, state->game.num_avail_grids);
    walrus_rhi_submit(0, state->grid_shader, WR_RHI_DISCARD_ALL);
}

void on_tick(Walrus_App *app, float dt)
{
    Romantik_GameState *state = walrus_app_userdata(app);
    game_state_tick(state, dt);
}

void on_event(Walrus_App *app, Walrus_Event *e)
{
    walrus_unused(app);

    if (e->type == WR_EVENT_TYPE_RESOLUTION) {
        walrus_rhi_set_view_rect(0, 0, 0, e->resolution.width, e->resolution.height);
        mat4 projection;
        glm_perspective(glm_rad(45.0), (float)e->resolution.width / e->resolution.height, 0.1, 1000.0, projection);
        walrus_rhi_set_view_transform(0, NULL, projection);
    }
    else if (e->type == WR_EVENT_TYPE_BUTTON) {
        if (e->button.device == WR_INPUT_KEYBOARD && e->button.button == WR_KEY_ESCAPE) {
            walrus_engine_exit();
        }
    }
}

Walrus_AppError on_init(Walrus_App *app)
{
    Romantik_GameState *state = walrus_app_userdata(app);

    game_state_init(state);
    return WR_APP_SUCCESS;
}

int main(void)
{
    Walrus_EngineOption opt;
    opt.window_title  = "romantik";
    opt.window_width  = 1440;
    opt.window_height = 900;
    opt.window_flags  = WR_WINDOW_FLAG_VSYNC | WR_WINDOW_FLAG_OPENGL;
    opt.minfps        = 30.f;

    Walrus_App *app = walrus_app_create(walrus_malloc(sizeof(Romantik_GameState)));
    walrus_app_set_init(app, on_init);
    walrus_app_set_tick(app, on_tick);
    walrus_app_set_render(app, on_render);
    walrus_app_set_event(app, on_event);

    walrus_engine_init_run(&opt, app);

    return 0;
}
