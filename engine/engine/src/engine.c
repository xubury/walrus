#include <engine.h>
#include <type.h>
#include <sys.h>
#include <log.h>
#include <event.h>
#include <rhi.h>

#include "app_impl.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <macro.h>
#include <platform.h>
#include <math.h>

struct _Engine {
    EngineOption opt;
    Window      *window;
    App         *app;
    Input       *input;
    bool         quit;
};

static Engine *s_engine = NULL;

#if PLATFORM == PLATFORM_WASM

typedef void (*WajsLoopCallback)(void);
typedef void (*WajsLoopEndCallback)(void);
typedef void (*WajsShutdownCallback)(void);

void wajs_set_main_loop(WajsLoopCallback loop, WajsLoopEndCallback loop_end);
void wajs_set_shutdown(WajsShutdownCallback shutdown);

bool __engine_should_close(void)
{
    return s_engine->quit;
}

#endif

static EngineError register_service(void)
{
    EngineOption *opt = &s_engine->opt;

    event_init();

    s_engine->input = inputs_create();
    if (s_engine->input == NULL) {
        return ENGINE_INIT_INPUT_ERROR;
    }
    s_engine->window = window_create(opt->window_title, opt->window_width, opt->window_height, opt->window_flags);
    if (s_engine->window == NULL) {
        return ENGINE_INIT_WINDOW_ERROR;
    }

    if (rhi_init(RHI_FLAG_BACKEND_OPENGL) != RHI_SUCCESS) {
        return ENGINE_INIT_RHI_ERROR;
    }

    return ENGINE_SUCCESS;
}

static void release_service(void)
{
    inputs_destroy(s_engine->input);
    window_destroy(s_engine->window);
    event_shutdown();
}

static void event_process(void)
{
    Event   e;
    i32     ret;
    App    *app    = s_engine->app;
    Input  *input  = s_engine->input;
    Window *window = s_engine->window;

    window_poll_events(window);
    while ((ret = event_poll(&e)) == EVENT_SUCCESS) {
        switch (e.type) {
            case EVENT_TYPE_AXIS: {
                AxisEvent *axis = &e.axis;
                if (axis->device == INPUT_MOUSE) {
                    input_set_axis(input->mouse, axis->axis, axis->x, axis->y, axis->z, axis->mods);
                }
            } break;
            case EVENT_TYPE_BUTTON: {
                ButtonEvent *btn = &e.button;
                if (btn->device == INPUT_MOUSE) {
                    input_set_button(input->mouse, btn->button, btn->state, btn->mods);
                }
                else if (btn->device == INPUT_KEYBOARD) {
                    input_set_button(input->keyboard, btn->button, btn->state, btn->mods);
                }
            } break;
            case EVENT_TYPE_RESOLUTION: {
                rhi_set_resolution(e.resolution.width, e.resolution.height);
            } break;
            case EVENT_TYPE_EXIT: {
                engine_exit();
            } break;
            default:
                break;
        }
        app->event(app, &e);
    }
}

static void engine_frame(void)
{
    App          *app    = s_engine->app;
    Input        *input  = s_engine->input;
    Window       *window = s_engine->window;
    EngineOption *opt    = &s_engine->opt;
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

    rhi_frame();

    window_swap_buffers(window);
}

char const *engine_error_msg(EngineError err)
{
    char const *err_msg[] = {"No error", "Fail to create window", rhi_error_msg(), "Fail to create input"};
    return err_msg[err];
}

void engine_init_run(EngineOption *opt, App *app)
{
    i32 err = engine_init(opt);
    if (err == ENGINE_SUCCESS) {
        engine_run(app);
    }
    else {
        log_error("%s", engine_error_msg(err));
    }
}

EngineError engine_init(EngineOption *opt)
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

    i32 error = ENGINE_SUCCESS;

    error = register_service();

    if (error != ENGINE_SUCCESS) {
        engine_shutdown();
    }

    return error;
}

void engine_shutdown(void)
{
    ASSERT_MSG(s_engine != NULL, "Engine should be initialized first");
    engine_exit();

    release_service();

    free(s_engine);
    s_engine = NULL;
}

static AppError app_init(App *app)
{
    AppError err = app->init(app);
    if (err == APP_SUCCESS) {
        s_engine->quit = false;
        s_engine->app  = app;
    }
    return err;
}

static void app_shutdown(void)
{
    App *app = s_engine->app;
    app->shutdown(app);
    s_engine->app = NULL;
}

AppError engine_run(App *app)
{
    ASSERT_MSG(s_engine != NULL, "Engine should be initialized first");
    ASSERT_MSG(app != NULL, "App can't be NULL!");

    // If engine has running app, exit the app first
    if (s_engine->app != NULL) {
        engine_exit();
    }

    AppError err = app_init(app);

    if (err == APP_SUCCESS) {
        // On wasm platfrom, loop is send to js process
#if PLATFORM == PLATFORM_WASM
        wajs_set_main_loop(engine_frame, app_shutdown);
#else
        // For other native platform, loop in current process
        while (!s_engine->quit) {
            engine_frame();
        }

        app_shutdown();
#endif
    }

    return err;
}

App *engine_exit(void)
{
    ASSERT_MSG(s_engine != NULL, "Engine should be initialized first");

    s_engine->quit = true;

    return s_engine->app;
}

Window *engine_get_window(void)
{
    return s_engine->window;
}

Input *engine_get_input(void)
{
    return s_engine->input;
}
