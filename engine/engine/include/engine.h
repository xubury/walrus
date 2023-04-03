#pragma once

#include <app.h>

typedef struct {
    i32 window_width;
    i32 window_height;
    f32 minfps;
} EngineOption;

struct _Engine {
    EngineOption opt;
    App         *app;
};

void engine_init(EngineOption *opt);

void engine_run(App *app);

void engine_destroy(void);
