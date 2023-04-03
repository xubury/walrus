#pragma once

#include <app.h>

struct _App {
    AppInitCallback    init;
    AppDestroyCallback destroy;
    AppTickCallback    tick;
    AppRenderCallback  render;
    AppEventCallback   event;

    void *userdata;
};
