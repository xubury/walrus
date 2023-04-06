#pragma once

#include <app.h>
#include <window.h>
#include <input.h>

typedef struct {
    i32 window_width;
    i32 window_height;
    f32 window_flags;
    f32 minfps;
} EngineOption;

typedef struct _Engine Engine;

typedef enum {
    ENGINE_SUCCESS = 0,

    ENGINE_INIT_WINDOW_ERROR
} EngineError;

// Helper function to init engine, runs an app and shutdown engine.
void engine_init_run(EngineOption *opt, App *app);

// Initialize engine
i32 engine_init(EngineOption *opt);

// Shutdown engine
void engine_shutdown(void);

// Run an app
void engine_run(App *app);

// Exit current app, return exited app
App *engine_exit(void);

// Get engine main window
Window *engine_get_window(void);

// Get engine input
Input *engine_get_input(void);
