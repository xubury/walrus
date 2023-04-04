#pragma once

#include <app.h>

struct _App {
    AppInitCallback     init;
    AppShutdownCallback shutdown;
    AppTickCallback     tick;
    AppRenderCallback   render;
    AppEventCallback    event;

    void *userdata;
};
