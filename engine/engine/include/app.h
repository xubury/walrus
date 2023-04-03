#pragma once

#include <type.h>
#include <event.h>

typedef struct _Engine Engine;

typedef void (*AppInitCallback)(Engine *engine);

typedef void (*AppTickCallback)(f32);

typedef void (*AppRenderCallback)(void);

typedef void (*AppEventCallback)(Event *);

typedef struct _App App;

App *app_alloc(void);

void app_free(App *app);

void app_set_init(App *app, AppInitCallback init);

void app_set_tick(App *app, AppTickCallback tick);

void app_set_render(App *app, AppRenderCallback render);

void app_set_event(App *app, AppEventCallback event);
