#pragma once

#include <app.h>

typedef struct {
    i32 window_width;
    i32 window_height;
    f32 minfps;
} EngineOption;

struct _Engine {
    EngineOption opt;
};

void engine_init(EngineOption *opt);

void engine_destroy(void);

void engine_run(App *app);

Engine *engine_get(void);
