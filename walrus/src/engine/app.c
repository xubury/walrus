#include <engine/app.h>
#include <core/macro.h>
#include "app_impl.h"

#include <stdlib.h>

static void dummy(Walrus_App *app)
{
    walrus_unused(app);
}

static Walrus_AppError dummyinit(Walrus_App *app)
{
    walrus_unused(app);

    return WR_APP_SUCCESS;
}

static void dummytick(Walrus_App *app, f32 t)
{
    walrus_unused(app);
    walrus_unused(t);
}

static void dummyevent(Walrus_App *app, Walrus_Event *e)
{
    walrus_unused(app);
    walrus_unused(e);
}

Walrus_App *walrus_app_create(void *userdata)
{
    Walrus_App *app = malloc(sizeof(Walrus_App));

    app->init     = dummyinit;
    app->shutdown = dummy;
    app->tick     = dummytick;
    app->render   = dummy;
    app->event    = dummyevent;

    app->userdata = userdata;

    return app;
}

void walrus_app_destroy(Walrus_App *app)
{
    free(app);
}

void walrus_app_set_userdata(Walrus_App *app, void *userdata)
{
    app->userdata = userdata;
}

void *walrus_app_userdata(Walrus_App *app)
{
    return app->userdata;
}

void walrus_app_set_init(Walrus_App *app, Walrus_AppInitCallback init)
{
    app->init = init;
}

void walrus_app_set_shutdown(Walrus_App *app, Walrus_AppShutdownCallback shutdown)
{
    app->shutdown = shutdown;
}

void walrus_app_set_tick(Walrus_App *app, Walrus_AppTickCallback tick)
{
    app->tick = tick;
}

void walrus_app_set_render(Walrus_App *app, Walrus_AppRenderCallback render)
{
    app->render = render;
}

void walrus_app_set_event(Walrus_App *app, Walrus_AppEventCallback event)
{
    app->event = event;
}
