#include <core/macro.h>
#include <core/math.h>
#include <core/memory.h>
#include <engine/engine.h>
#include <rhi/rhi.h>

#include "game_state.h"

void on_render(Walrus_App *app)
{
    Romantik_GameState *state = app->userdata;
    game_state_render(state);
}

void on_tick(Walrus_App *app, float dt)
{
    Romantik_GameState *state = app->userdata;
    game_state_tick(state, dt);
}

void on_event(Walrus_App *app, Walrus_Event *e)
{
    walrus_unused(app);

    if (e->type == WR_EVENT_TYPE_RESOLUTION) {
        i32 const width  = e->resolution.width;
        i32 const height = e->resolution.height;
        walrus_rhi_set_view_rect(0, 0, 0, width, height);
        walrus_rhi_set_view_rect(1, width - width * 0.3 + 100, height - height * 0.3 - 100, width * 0.3, height * 0.3);
        mat4 projection;
        glm_perspective(glm_rad(45.0), (float)width / height, 0.1, 1000.0, projection);
        walrus_rhi_set_view_transform(0, NULL, projection);
        walrus_rhi_set_view_transform(1, NULL, projection);
    }
    else if (e->type == WR_EVENT_TYPE_BUTTON) {
        if (e->button.device == WR_INPUT_KEYBOARD && e->button.button == WR_KEY_ESCAPE) {
            walrus_engine_exit();
        }
    }
}

Walrus_AppError on_init(Walrus_App *app)
{
    Romantik_GameState *state = app->userdata;

    game_state_init(state);
    return WR_APP_SUCCESS;
}

int main(void)
{
    Walrus_App app = {.init     = on_init,
                      .tick     = on_tick,
                      .render   = on_render,
                      .event    = on_event,
                      .userdata = walrus_malloc(sizeof(Romantik_GameState))};

    walrus_engine_init_run("romantik", 1440, 900, &app);

    return 0;
}
