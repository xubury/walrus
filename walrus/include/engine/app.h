#pragma once

#include <core/type.h>
#include <engine/event.h>

typedef enum {
    WR_APP_SUCCESS = 0,

    WR_APP_INIT_FAIL = 1,  // init app failed
    WR_APP_NO_ENGINE = 2,  // engine is not init
} Walrus_AppError;

typedef struct Walrus_App Walrus_App;

typedef Walrus_AppError (*Walrus_AppInitCallback)(Walrus_App *);

typedef void (*Walrus_AppShutdownCallback)(Walrus_App *);

typedef void (*Walrus_AppTickCallback)(Walrus_App *, f32);

typedef void (*Walrus_AppRenderCallback)(Walrus_App *);

typedef void (*Walrus_AppEventCallback)(Walrus_App *, Walrus_Event *);

struct Walrus_App {
    Walrus_AppInitCallback     init;
    Walrus_AppShutdownCallback shutdown;
    Walrus_AppTickCallback     tick;
    Walrus_AppRenderCallback   render;
    Walrus_AppEventCallback    event;

    void *userdata;
};
