#include <engine/engine.h>
#include <engine/event.h>
#include <engine/batch_renderer.h>
#include <rhi/rhi.h>
#include <core/type.h>
#include <core/sys.h>
#include <core/log.h>
#include <core/assert.h>
#include <core/math.h>
#include <core/platform.h>
#include <core/memory.h>
#include <core/thread.h>
#include <core/mutex.h>

#include "app_impl.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

struct _Walrus_Engine {
    Walrus_EngineOption opt;
    Walrus_Mutex       *log_mutex;
    Walrus_Thread      *render;
    Walrus_Window      *window;
    Walrus_App         *app;
    Walrus_Input       *input;
    bool                quit;
};

static Walrus_Engine *s_engine = NULL;

#if WR_PLATFORM == WR_PLATFORM_WASM

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

static void log_lock_fn(bool lock, void *userdata)
{
    Walrus_Mutex *mutex = (Walrus_Mutex *)userdata;
    lock ? walrus_mutex_lock(mutex) : walrus_mutex_unlock(mutex);
}

static void setup_window(void)
{
    walrus_window_make_current(s_engine->window);
    walrus_window_set_vsync(s_engine->window, s_engine->opt.window_flags & WR_WINDOW_FLAG_VSYNC);
}

static Walrus_EngineError register_service(void)
{
    Walrus_EngineOption *opt = &s_engine->opt;

    s_engine->log_mutex = walrus_mutex_create();
    walrus_log_set_lock(log_lock_fn, s_engine->log_mutex);

    walrus_event_init();

    s_engine->input = walrus_inputs_create();
    if (s_engine->input == NULL) {
        return WR_ENGINE_INIT_INPUT_ERROR;
    }
    s_engine->window =
        walrus_window_create(opt->window_title, opt->window_width, opt->window_height, opt->window_flags);
    if (s_engine->window == NULL) {
        return WR_ENGINE_INIT_WINDOW_ERROR;
    }

    if (opt->single_thread) {
        setup_window();
    }

    Walrus_RhiCreateInfo info;
    if (opt->window_flags & WR_WINDOW_FLAG_OPENGL) {
        info.flags |= WR_RHI_FLAG_OPENGL;
    }
    info.single_thread = opt->single_thread;

    if (walrus_rhi_init(&info) != WR_RHI_SUCCESS) {
        return WR_ENGINE_INIT_RHI_ERROR;
    }
    walrus_rhi_set_resolution(opt->window_width, opt->window_height);

    walrus_batch_render_init();

    return WR_ENGINE_SUCCESS;
}

static void release_service(void)
{
    walrus_batch_render_shutdown();
    walrus_rhi_shutdown();
    walrus_inputs_destroy(s_engine->input);
    walrus_window_destroy(s_engine->window);
    walrus_event_shutdown();
    walrus_mutex_destroy(s_engine->log_mutex);
    walrus_log_set_lock(NULL, NULL);
}

static void event_process(void)
{
    static Walrus_Event e;
    Walrus_App         *app    = s_engine->app;
    Walrus_Input       *input  = s_engine->input;
    Walrus_Window      *window = s_engine->window;

    walrus_window_poll_events(window);
    while (walrus_event_poll(&e) == WR_EVENT_SUCCESS) {
        switch (e.type) {
            case WR_EVENT_TYPE_AXIS: {
                Walrus_AxisEvent *axis = &e.axis;
                if (axis->device == WR_INPUT_MOUSE) {
                    walrus_input_set_axis(input->mouse, axis->axis, axis->x, axis->y, axis->z, axis->mods);
                }
            } break;
            case WR_EVENT_TYPE_BUTTON: {
                Walrus_ButtonEvent *btn = &e.button;
                if (btn->device == WR_INPUT_MOUSE) {
                    walrus_input_set_button(input->mouse, btn->button, btn->state, btn->mods);
                }
                else if (btn->device == WR_INPUT_KEYBOARD) {
                    walrus_input_set_button(input->keyboard, btn->button, btn->state, btn->mods);
                }
            } break;
            case WR_EVENT_TYPE_RESOLUTION: {
                walrus_rhi_set_resolution(e.resolution.width, e.resolution.height);
            } break;
            case WR_EVENT_TYPE_EXIT: {
                walrus_engine_exit();
            } break;
            default:
                break;
        }
        app->event(app, &e);
    }
}

static void engine_frame(void)
{
    Walrus_App          *app    = s_engine->app;
    Walrus_Window       *window = s_engine->window;
    Walrus_Input        *input  = s_engine->input;
    Walrus_EngineOption *opt    = &s_engine->opt;
    walrus_assert_msg(opt->minfps > 0, "Invalid min fps");
    walrus_assert_msg(app->tick != NULL, "Invalid tick function");
    walrus_assert_msg(app->render != NULL, "Invalid render function");
    walrus_assert_msg(app->event != NULL, "Invalid event function");

    f32 const max_spf = 1.0 / s_engine->opt.minfps;

    static f32 sec_elapesd = 0.f;
    static f32 input_timer = 0.f;
    static u64 last_ts     = 0;

    u64 const nw = walrus_sysclock(WR_SYS_CLOCK_UNIT_MILLSEC);

    if (last_ts > 0) {
        sec_elapesd = (nw - last_ts) * 1e-3;
    }
    last_ts = nw;

    input_timer += sec_elapesd;

    while (sec_elapesd > max_spf) {
        sec_elapesd -= max_spf;
        app->tick(app, max_spf);
    }

    if (sec_elapesd > 0) {
        app->tick(app, sec_elapesd);
    }

    app->render(app);

    if (input_timer > 1.0 / 60.0) {
        walrus_inputs_tick(input);
        input_timer = 0.f;
    }

    event_process();

    walrus_rhi_frame();

    if (opt->single_thread) {
        walrus_window_swap_buffers(window);
    }
}

char const *walrus_engine_error_msg(Walrus_EngineError err)
{
    switch (err) {
        case WR_ENGINE_SUCCESS:
            return "No error";
        case WR_ENGINE_INIT_WINDOW_ERROR:
            return "Fail to create window";
        case WR_ENGINE_INIT_RHI_ERROR:
            return walrus_rhi_error_msg();
        case WR_ENGINE_INIT_INPUT_ERROR:
            return "Fail to create input";
        case WR_ENGINE_RUN_APP_ERROR:
            return "Fail to run app";
    }
}

Walrus_AppError walrus_engine_init_run(char const *title, u32 width, u32 height, Walrus_App *app)
{
    Walrus_EngineOption opt;
    opt.window_title  = title;
    opt.window_width  = width;
    opt.window_height = height;
    opt.window_flags  = WR_WINDOW_FLAG_VSYNC | WR_WINDOW_FLAG_OPENGL;
    opt.minfps        = 30.f;
    opt.single_thread = false;

    Walrus_EngineError err = walrus_engine_init(&opt);

    if (err == WR_ENGINE_SUCCESS) {
        return walrus_engine_run(app);
    }
    else {
        walrus_error("%s", walrus_engine_error_msg(err));
    }

    return WR_APP_NO_ENGINE;
}

static i32 render_thread_fn(Walrus_Thread *thread, void *userdata)
{
    walrus_unused(thread);
    walrus_unused(userdata);

    Walrus_Window *window = NULL;
    while (window == NULL) {
        window = s_engine->window;
    }

    setup_window();

    while (walrus_rhi_render_frame(-1) != WR_RHI_RENDER_EXITING) {
        walrus_window_swap_buffers(window);
    }

    return 0;
}

Walrus_EngineError walrus_engine_init(Walrus_EngineOption *opt)
{
    s_engine = walrus_malloc(sizeof(Walrus_Engine));
    if (opt != NULL) {
        memcpy(&s_engine->opt, opt, sizeof(Walrus_EngineOption));
    }

    opt                = &s_engine->opt;
    opt->minfps        = walrus_max(opt->minfps, 1.0);
    opt->window_width  = walrus_max(opt->window_width, 1);
    opt->window_height = walrus_max(opt->window_height, 1);

    s_engine->app    = NULL;
    s_engine->window = NULL;
    s_engine->input  = NULL;
    s_engine->render = NULL;
    s_engine->quit   = true;

    if (!opt->single_thread) {
        s_engine->render = walrus_thread_create();
        walrus_thread_init(s_engine->render, render_thread_fn, NULL, 0);
    }

    Walrus_EngineError error = WR_ENGINE_SUCCESS;

    error = register_service();

    if (error != WR_ENGINE_SUCCESS) {
        walrus_engine_shutdown();
    }

    return error;
}

void walrus_engine_shutdown(void)
{
    walrus_assert_msg(s_engine != NULL, "Engine should be initialized first");
    walrus_engine_exit();

    release_service();

    if (s_engine->render != NULL) {
        walrus_thread_destroy(s_engine->render);
    }

    walrus_free(s_engine);
    s_engine = NULL;
}

static Walrus_AppError app_init(Walrus_App *app)
{
    Walrus_AppError err = app->init(app);
    if (err == WR_APP_SUCCESS) {
        s_engine->quit = false;
        s_engine->app  = app;
    }
    return err;
}

static void app_shutdown(void)
{
    Walrus_App *app = s_engine->app;
    app->shutdown(app);
    s_engine->app = NULL;
}

Walrus_AppError walrus_engine_run(Walrus_App *app)
{
    walrus_assert_msg(app != NULL, "App can't be NULL!");
    if (s_engine == NULL) {
        return WR_APP_NO_ENGINE;
    }

    // If engine has running app, exit the app first
    if (s_engine->app != NULL) {
        walrus_engine_exit();
    }

    Walrus_AppError err = app_init(app);

    if (err == WR_APP_SUCCESS) {
        // On wasm platfrom, loop is send to js process
#if WR_PLATFORM == WR_PLATFORM_WASM
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

Walrus_App *walrus_engine_exit(void)
{
    walrus_assert_msg(s_engine != NULL, "Engine should be initialized first");

    s_engine->quit = true;

    return s_engine->app;
}

Walrus_Window *walrus_engine_window(void)
{
    return s_engine->window;
}

Walrus_Input *walrus_engine_input(void)
{
    return s_engine->input;
}
