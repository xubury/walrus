#pragma once

#include <app.h>

struct _App {
    AppInitCallback   init;
    AppTickCallback   tick;
    AppRenderCallback render;
    AppEventCallback  event;
};

