#pragma once

#include <engine/app.h>
#include <engine/window.h>
#include <engine/input.h>

typedef struct {
    char const *window_title;
    i32         window_width;
    i32         window_height;
    i32         window_flags;
    f32         minfps;
} Walrus_EngineOption;

typedef struct _Walrus_Engine Walrus_Engine;

typedef enum {
    WR_ENGINE_SUCCESS = 0,

    WR_ENGINE_INIT_WINDOW_ERROR,
    WR_ENGINE_INIT_RHI_ERROR,
    WR_ENGINE_INIT_INPUT_ERROR,

    WR_ENGINE_RUN_APP_ERROR,
} Walrus_EngineError;

char const *walrus_engine_error_msg(Walrus_EngineError err);

// Helper function to init engine, runs an app and shutdown engine.
Walrus_AppError walrus_engine_init_run(Walrus_EngineOption *opt, Walrus_App *app);

// Initialize engine
Walrus_EngineError walrus_engine_init(Walrus_EngineOption *opt);

// Shutdown engine
void walrus_engine_shutdown(void);

// Run an app
Walrus_AppError walrus_engine_run(Walrus_App *app);

// Exit current app, return exited app
Walrus_App *walrus_engine_exit(void);

// Get engine main window
Walrus_Window *walrus_engine_window(void);

// Get engine input
Walrus_Input *walrus_engine_input(void);
