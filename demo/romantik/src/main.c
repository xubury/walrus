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
    game_state_render(state);
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
    u8 bin = 0x81;
    walrus_trace("0b%d%d%d%d%d%d%d%d", (bin >> 7) & 1, (bin >> 6) & 1, (bin >> 5) & 1, (bin >> 4) & 1, (bin >> 3) & 1,
                 (bin >> 2) & 1, (bin >> 1) & 1, (bin >> 0) & 1);
    bin = walrus_u8rol(bin, 2);
    walrus_trace("0b%d%d%d%d%d%d%d%d", (bin >> 7) & 1, (bin >> 6) & 1, (bin >> 5) & 1, (bin >> 4) & 1, (bin >> 3) & 1,
                 (bin >> 2) & 1, (bin >> 1) & 1, (bin >> 0) & 1);
    Walrus_EngineOption opt;
    opt.window_title  = "romantik";
    opt.window_width  = 1440;
    opt.window_height = 900;
    opt.window_flags  = WR_WINDOW_FLAG_OPENGL;
    opt.minfps        = 30.f;

    Walrus_App *app = walrus_app_create(walrus_malloc(sizeof(Romantik_GameState)));
    walrus_app_set_init(app, on_init);
    walrus_app_set_tick(app, on_tick);
    walrus_app_set_render(app, on_render);
    walrus_app_set_event(app, on_event);

    walrus_engine_init_run(&opt, app);

    return 0;
}
