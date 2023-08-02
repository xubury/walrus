#pragma once

#include <core/log.h>
#include <engine/app.h>
#include <engine/window.h>
#include <engine/input.h>
#include <engine/systems/model_system.h>
#include <rhi/rhi.h>

#include <flecs.h>

typedef struct {
    char const       *log_file;
    u32               log_file_level;
    char const       *window_title;
    u32               window_flags;
    Walrus_Resolution resolution;
    f32               minfps;
    bool              single_thread;
    char const       *shader_folder;
    u8                thread_pool_size;
} Walrus_EngineOption;

typedef struct {
    Walrus_Window *window;
    Walrus_Input  *input;
    ecs_world_t   *ecs;
    Walrus_System *model;
} Walrus_EngineVars;

typedef struct Walrus_Engine Walrus_Engine;

typedef enum {
    WR_ENGINE_SUCCESS = 0,

    WR_ENGINE_INIT_WINDOW_ERROR,
    WR_ENGINE_INIT_RHI_ERROR,
    WR_ENGINE_INIT_INPUT_ERROR,

    WR_ENGINE_RUN_APP_ERROR,
} Walrus_EngineError;

char const *walrus_engine_error_msg(Walrus_EngineError err);

// Helper function to init engine, runs an app and shutdown engine.
Walrus_AppError walrus_engine_init_run(char const *title, u32 width, u32 height, Walrus_App *app);

// Initialize engine
Walrus_EngineError walrus_engine_init(Walrus_EngineOption *opt);

// Shutdown engine
void walrus_engine_shutdown(void);

// Run an app
Walrus_AppError walrus_engine_run(Walrus_App *app);

// Exit current app, return exited app
Walrus_App *walrus_engine_exit(void);

Walrus_EngineVars *walrus_engine_vars(void);

void walrus_engine_add_system(Walrus_System *sys, char const *name);
