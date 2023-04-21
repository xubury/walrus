#pragma once

#include <rhi/rhi.h>

#include "camera.h"
#include "game_logic.h"

typedef struct {
    Walrus_ProgramHandle pick_shader;
    Walrus_ProgramHandle map_shader;
    Walrus_ProgramHandle grid_shader;
    Walrus_UniformHandle u_texture;
    Walrus_UniformHandle u_color;
    Walrus_BufferHandle  buffer;

    Walrus_BufferHandle  index_buffer;
    Walrus_LayoutHandle  layout;
    Walrus_TextureHandle texture;

    Walrus_LayoutHandle ins_layout;
    Walrus_BufferHandle placed_buffer;
    Walrus_BufferHandle avail_buffer;
    Walrus_BufferHandle queue_buffer;

    mat4 model;

    CameraData cam;

    bool hide_picker;

    Romantik_Game game;
} Romantik_GameState;

void game_state_init(Romantik_GameState *state);

void game_state_tick(Romantik_GameState *state, f32 dt);

void game_state_render(Romantik_GameState *state);
