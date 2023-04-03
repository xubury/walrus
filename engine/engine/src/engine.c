#include <engine.h>
#include <type.h>
#include <sys.h>
#include <event.h>

#include "app_impl.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <macro.h>
#include <math.h>

typedef void (*WajsLoopCallback)(void);

void wajs_setup_gl_context(i32 width, i32 height);

void wajs_set_main_loop(WajsLoopCallback loop);

Engine *s_engine = NULL;

static void event_process(void)
{
    // Event queue test
    Event e;
    i32   ret;
    while ((ret = event_poll(&e)) == EVENT_SUCCESS) {
        s_engine->app->event(&e);
    }
}

static void engine_loop(void)
{
    App          *app = s_engine->app;
    EngineOption *opt = &s_engine->opt;
    ASSERT(opt->minfps > 0, "Invalid min fps");
    ASSERT(app->tick != NULL, "Invalid tick function");
    ASSERT(app->render != NULL, "Invalid render function");

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
        app->tick(max_spf);
    }

    if (sec_elapesd > 0) {
        app->tick(sec_elapesd);
    }

    app->render();

    event_process();
}

void engine_init(EngineOption *opt)
{
    opt->minfps        = fmax(opt->minfps, 1.0);
    opt->window_width  = fmax(opt->window_width, 1);
    opt->window_height = fmax(opt->window_height, 1);

    s_engine = malloc(sizeof(Engine));
    memcpy(&s_engine->opt, opt, sizeof(EngineOption));

    wajs_setup_gl_context(opt->window_width, opt->window_height);
    event_init();
}

void engine_run(App *app)
{
    s_engine->app = app;
    app->init(s_engine);

    wajs_set_main_loop(engine_loop);
}

void engine_destroy(void)
{
    // do nothing, handle by js side
}
