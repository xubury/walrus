#include <app.h>
#include <macro.h>
#include "app_impl.h"

#include <stdlib.h>

static void dummy(void)
{
}

static void dummyinit(Engine *engine)
{
    UNUSED(engine);
}

static void dummytick(f32 t)
{
    UNUSED(t);
}

static void dummyevent(Event *e)
{
    UNUSED(e);
}

App *app_alloc(void)
{
    App *app    = malloc(sizeof(App));
    app->init   = dummyinit;
    app->tick   = dummytick;
    app->render = dummy;
    app->event  = dummyevent;

    return app;
}

void app_free(App *app)
{
    free(app);
}

void app_set_init(App *app, AppInitCallback init)
{
    app->init = init;
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
