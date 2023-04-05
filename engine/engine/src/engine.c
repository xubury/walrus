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

#if PLATFORM == PLATFORM_WASI

typedef void (*WajsLoopCallback)(void);
typedef void (*WajsShutdownCallback)(void);

void wajs_set_main_loop(WajsLoopCallback loop);
void wajs_set_shutdown(WajsShutdownCallback shutdown);

#endif

struct _Engine {
    EngineOption opt;
    Window      *window;
    App         *app;
    Input       *input;
    bool         quit;
};

static Engine *s_engine = NULL;

static void register_service(void)
{
    EngineOption *opt = &s_engine->opt;

    event_init();
    s_engine->window = window_create(opt->window_width, opt->window_height, opt->window_flags);
    s_engine->input  = inputs_create();
}

static void release_service(void)
{
    inputs_destroy(s_engine->input);
    window_destroy(s_engine->window);
    event_shutdown();
}

static void event_process(void)
{
    Event e;
    i32   ret;
    App  *app = s_engine->app;
    while ((ret = event_poll(&e)) == EVENT_SUCCESS) {
        app->event(app, &e);
    }
}

static void _engine_shutdown(void)
{
    ASSERT_MSG(s_engine != NULL, "Engine should be initialized first");
    engine_exit();

    release_service();

    free(s_engine);
    s_engine = NULL;
}

static void engine_frame(void)
{
    if (s_engine->quit) return;

    App          *app   = s_engine->app;
    Input        *input = s_engine->input;
    EngineOption *opt   = &s_engine->opt;
    ASSERT_MSG(opt->minfps > 0, "Invalid min fps");
    ASSERT_MSG(app->tick != NULL, "Invalid tick function");
    ASSERT_MSG(app->render != NULL, "Invalid render function");
    ASSERT_MSG(app->event != NULL, "Invalid event function");

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

    inputs_tick(input);

    event_process();
}

void engine_init_run(EngineOption *opt, App *app)
{
    engine_init(opt);

    engine_run(app);

    engine_shutdown();
}

void engine_init(EngineOption *opt)
{
    s_engine = malloc(sizeof(Engine));
    if (opt != NULL) {
        memcpy(&s_engine->opt, opt, sizeof(EngineOption));
    }

    opt                = &s_engine->opt;
    opt->minfps        = fmax(opt->minfps, 1.0);
    opt->window_width  = fmax(opt->window_width, 1);
    opt->window_height = fmax(opt->window_height, 1);

    s_engine->app  = NULL;
    s_engine->quit = true;

    register_service();

    // On wasm platfrom, shutdown is sent to js process
#if PLATFORM == PLATFORM_WASI
    wajs_set_shutdown(_engine_shutdown);
#endif
}

void engine_run(App *app)
{
    ASSERT_MSG(s_engine != NULL, "Engine should be initialized first");

    // If engine has running app, exit the app first
    if (s_engine->app != NULL) {
        engine_exit();
    }

    if (app->init(app) == INIT_SUCCESS) {
        s_engine->quit = false;
        s_engine->app  = app;
        // On wasm platfrom, loop is send to js process
#if PLATFORM == PLATFORM_WASI
        wajs_set_main_loop(engine_frame);
#else
        // For other native platform, loop in current process
        while (!s_engine->quit) {
            engine_frame();
        }

        app->shutdown(app);

        s_engine->app = NULL;
#endif
    }
}

App *engine_exit(void)
{
    ASSERT_MSG(s_engine != NULL, "Engine should be initialized first");

    s_engine->quit = true;

    return s_engine->app;
}

void engine_shutdown(void)
{
#if PLATFORM != PLATFORM_WASI
    _engine_shutdown();
#else
    // wasm platform's loop & shutdown is dealt by js side, so do nothing here
#endif
}

Window *engine_get_window(void)
{
    return s_engine->window;
}

Input *engine_get_input(void)
{
    return s_engine->input;
}
