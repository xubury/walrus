#include <app.h>
#include <macro.h>
#include "app_impl.h"

#include <stdlib.h>

static void dummy(App *app)
{
    UNUSED(app);
}

static void dummyinit(App *app)
{
    UNUSED(app);
}

static void dummytick(App *app, f32 t)
{
    UNUSED(app) UNUSED(t);
}

static void dummyevent(App *app, Event *e)
{
    UNUSED(app) UNUSED(e);
}

App *app_create(void)
{
    App *app = malloc(sizeof(App));

    app->init    = dummyinit;
    app->destroy = dummy;
    app->tick    = dummytick;
    app->render  = dummy;
    app->event   = dummyevent;

    app->userdata = NULL;

    return app;
}

void app_destroy(App *app)
{
    app->destroy(app);
    free(app);
}

void app_set_userdata(App *app, void *userdata)
{
    app->userdata = userdata;
}

void *app_get_userdata(App *app)
{
    return app->userdata;
}

void app_set_init(App *app, AppInitCallback init)
{
    app->init = init;
}

void app_set_destroy(App *app, AppDestroyCallback destroy)
{
    app->destroy = destroy;
}

void app_set_tick(App *app, AppTickCallback tick)
{
    app->tick = tick;
}

void app_set_render(App *app, AppRenderCallback render)
{
    app->render = render;
}

void app_set_event(App *app, AppEventCallback event)
{
    app->event = event;
}
