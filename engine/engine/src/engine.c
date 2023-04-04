#include <engine.h>
#include <type.h>
#include <sys.h>
#include <event.h>

#include "app_impl.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <macro.h>
#include <platform.h>
#include <math.h>

typedef void (*WajsLoopCallback)(void);

#if PLATFORM == PLATFORM_WASI
void wajs_set_main_loop(WajsLoopCallback loop);
#endif

struct _Engine {
    EngineOption opt;
    Window      *window;
    App         *app;
    bool         quit;
};

static Engine *s_engine = NULL;

static void event_process(void)
{
    Event e;
    i32   ret;
    App  *app = s_engine->app;
    while ((ret = event_poll(&e)) == EVENT_SUCCESS) {
        app->event(app, &e);
    }
}

static void engine_loop(void)
{
    App          *app = s_engine->app;
    EngineOption *opt = &s_engine->opt;
    ASSERT(opt->minfps > 0, "Invalid min fps");
    ASSERT(app->tick != NULL, "Invalid tick function");
    ASSERT(app->render != NULL, "Invalid render function");
    ASSERT(app->event != NULL, "Invalid event function");

    f32 const max_spf = 1.0 / s_engine->opt.minfps;

    static f32 sec_elapesd = 0.f;
    static u64 last_ts     = 0;

    u64 const nw = sysclock(SYS_CLOCK_UNIT_MILLSEC);

    if (last_ts > 0) {
        sec_elapesd = (nw - last_ts) * 1e-3;
    }
    last_ts = nw;

    while (sec_elapesd > max_spf) {
        sec_elapesd -= max_spf;
        app->tick(app, max_spf);
    }

    if (sec_elapesd > 0) {
        app->tick(app, sec_elapesd);
    }

    app->render(app);

    event_process();
}

void engine_init(EngineOption *opt)
{
    opt->minfps        = fmax(opt->minfps, 1.0);
    opt->window_width  = fmax(opt->window_width, 1);
    opt->window_height = fmax(opt->window_height, 1);

    event_init();

    s_engine = malloc(sizeof(Engine));

    memcpy(&s_engine->opt, opt, sizeof(EngineOption));
    s_engine->app    = NULL;
    s_engine->window = window_create(opt->window_width, opt->window_height, opt->window_flags);
    s_engine->quit   = true;
}

void engine_run(App *app)
{
    ASSERT(s_engine != NULL, "Engine should be initialize first");

    s_engine->quit = false;

    // If engine has running app, exit the app first
    if (s_engine->app != NULL) {
        engine_exit();
    }

    s_engine->app = app;

    app->init(app);

#if PLATFORM == PLATFORM_WASI
    wajs_set_main_loop(engine_loop);
#else
    while (!s_engine->quit) {
        engine_loop();
    }
    engine_exit();
#endif
}

void engine_exit(void)
{
    ASSERT(s_engine != NULL, "Engine should be initialize first");

    App *app      = s_engine->app;
    s_engine->app = NULL;

    app->shutdown(app);
}

void engine_shutdown(void)
{
    ASSERT(s_engine != NULL, "Engine should be initialize first");

#if PLATFORM != PLATFORM_WASI
    engine_exit(s_app);

    event_shutdown();
    window_destroy(s_engine->window);

    free(s_engine);
    s_engine = NULL;
#endif
}

Window *engine_get_window(void)
{
    return s_engine->window;
}
