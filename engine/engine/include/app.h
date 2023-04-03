#pragma once

#include <type.h>
#include <event.h>

typedef struct _App App;

typedef void (*AppInitCallback)(App *);

typedef void (*AppDestroyCallback)(App *);

typedef void (*AppTickCallback)(App *, f32);

typedef void (*AppRenderCallback)(App *);

typedef void (*AppEventCallback)(App *, Event *);

App *app_create(void);

void app_destroy(App *app);

void app_set_userdata(App *app, void *userdata);

void *app_get_userdata(App *app);

void app_set_init(App *app, AppInitCallback init);

void app_set_destroy(App *app, AppDestroyCallback destroy);

void app_set_tick(App *app, AppTickCallback tick);

void app_set_render(App *app, AppRenderCallback render);

void app_set_event(App *app, AppEventCallback event);
