#pragma once

#include <engine/app.h>

struct Walrus_App {
    Walrus_AppInitCallback     init;
    Walrus_AppShutdownCallback shutdown;
    Walrus_AppTickCallback     tick;
    Walrus_AppRenderCallback   render;
    Walrus_AppEventCallback    event;

    void *userdata;
};
