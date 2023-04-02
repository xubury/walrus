#include <engine.h>
#include <type.h>
#include <sys.h>
#include <event.h>
#include <wajs_window.h>

#include <stdio.h>

f32 const min_fps     = 30.f;
f32 const max_spf     = 1.0 / min_fps;
f32       sec_elapesd = 0.f;

void event_process(void)
{
    // Event queue test
    Event e;
    i32   ret;
    while ((ret = event_poll(&e)) == EVENT_SUCCESS) {
        if (e.type == EVENT_TYPE_AXIS) {
            printf("mouse move: %d, %d\n", e.axis.x, e.axis.y);
        }
        else if (e.type == EVENT_TYPE_BUTTON) {
            printf("key: %d\n", e.button.code);
        }
    }
}

void loop(EngineRenderCallback render, EngineTickCallback tick)
{
    static u64 last_ts = 0;
    const u64  nw      = sysclock(SYS_CLOCK_UNIT_MILLSEC);
    if (last_ts > 0) {
        sec_elapesd = (nw - last_ts) * 1e-3;
    }
    last_ts = nw;

    while (sec_elapesd > max_spf) {
        sec_elapesd -= max_spf;
        tick(max_spf);
    }
    tick(sec_elapesd);

    render();

    event_process();

}

void engine_run(EngineRenderCallback render, EngineTickCallback tick)
{
    wajs_set_main_loop(loop, render, tick);
}
