#include <wajs_gl.h>

#include <sys.h>

static f32 s_frametime = 0;

f32 wajs_get_frametime(void)
{
    return s_frametime;
}

void __wajs_update_frametime(void)
{
    static f32 lastts = 0.f;
    f32        nw     = sysclock(SYS_CLOCK_UNIT_MILLSEC);
    s_frametime       = nw - lastts;
    lastts            = nw;
}
