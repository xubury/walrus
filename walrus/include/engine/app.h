#pragma once

#include <core/type.h>
#include <engine/event.h>

typedef enum {
    WR_APP_SUCCESS = 0,

    WR_APP_INIT_FAIL = 1,
} Walrus_AppError;

typedef struct _Walrus_App Walrus_App;

typedef Walrus_AppError (*Walrus_AppInitCallback)(Walrus_App *);

typedef void (*Walrus_AppShutdownCallback)(Walrus_App *);

typedef void (*Walrus_AppTickCallback)(Walrus_App *, f32);

typedef void (*Walrus_AppRenderCallback)(Walrus_App *);

typedef void (*Walrus_AppEventCallback)(Walrus_App *, Walrus_Event *);

Walrus_App *walrus_app_create(void *userdata);

void walrus_app_destroy(Walrus_App *app);

void walrus_app_set_userdata(Walrus_App *app, void *userdata);

void *walrus_app_userdata(Walrus_App *app);

void walrus_app_set_init(Walrus_App *app, Walrus_AppInitCallback init);

void walrus_app_set_shutdown(Walrus_App *app, Walrus_AppShutdownCallback destroy);

void walrus_app_set_tick(Walrus_App *app, Walrus_AppTickCallback tick);

void walrus_app_set_render(Walrus_App *app, Walrus_AppRenderCallback render);

void walrus_app_set_event(Walrus_App *app, Walrus_AppEventCallback event);
