#include <wajs_gl.h>

#include <sys.h>

static f32 s_frametime = 0;

f32 wajsGetFrameTime()
{
    return s_frametime;
}

void __wajsUpdateFrameTime()
{
    static f32 lastts = 0.f;
    f32        nw     = sysclock(SYS_CLOCK_UNIT_MILLSEC);
    s_frametime       = nw - lastts;
    lastts            = nw;
}
