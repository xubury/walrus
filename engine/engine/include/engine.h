#pragma once

#include <app.h>
#include <window.h>

typedef struct {
    i32 window_width;
    i32 window_height;
    f32 window_flags;
    f32 minfps;
} EngineOption;

struct _Engine {
    EngineOption opt;
    Window      *window;
};

typedef struct _Engine Engine;

void engine_init(EngineOption *opt);

void engine_shutdown(void);

void engine_run(App *app);

Engine *engine_get(void);
