#include <engine/app.h>
#include <core/macro.h>
#include <core/memory.h>

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
    Walrus_App *app = walrus_malloc(sizeof(Walrus_App));

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
    walrus_free(app);
}
