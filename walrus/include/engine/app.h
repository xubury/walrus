#pragma once

#include <core/type.h>
#include <core/cpoly.h>
#include <engine/event.h>

typedef enum {
    WR_APP_SUCCESS = 0,

    WR_APP_INIT_FAIL = 1,  // init app failed
    WR_APP_NO_ENGINE = 2,  // engine is not init
} Walrus_AppError;

typedef struct Walrus_App Walrus_App;

typedef Walrus_AppError (*Walrus_AppInitCallback)(Walrus_App *);

typedef void (*Walrus_AppEventCallback)(Walrus_App *, Walrus_Event *);

struct Walrus_App {
    POLY_TABLE(Walrus_App, POLY_INTERFACE(on_app_init), POLY_INTERFACE(on_app_shutdown), POLY_INTERFACE(on_app_event),
               POLY_INTERFACE(on_app_render), POLY_INTERFACE(on_app_tick))
};

POLY_PROTOTYPE(Walrus_AppError, on_app_init, Walrus_App *)
POLY_PROTOTYPE(void, on_app_shutdown, Walrus_App *)
POLY_PROTOTYPE(void, on_app_tick, Walrus_App *, f32)
POLY_PROTOTYPE(void, on_app_render, Walrus_App *)
POLY_PROTOTYPE(void, on_app_event, Walrus_App *, Walrus_Event *)
