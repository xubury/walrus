#include <wajs_gl.h>

#include <stddef.h>
#include <time.h>

typedef struct timespec timespec;

static float unixTimeNanoseconds()
{
    timespec nw;
    clock_gettime(CLOCK_REALTIME, &nw);
    return nw.tv_nsec;
}

static float unixTimeMiliseconds()
{
    return unixTimeNanoseconds() / (1.0e6);
}

static float unixTimeSeoncds()
{
    return unixTimeNanoseconds() / (1.0e9);
}

static float s_frametime = 0;

float wajsGetFrameTime()
{
    return s_frametime;
}

void __wajsUpdateFrameTime()
{
    static float lastts = 0.f;
    float        nw     = unixTimeMiliseconds();
    s_frametime         = nw - lastts;
    lastts              = nw;
}
