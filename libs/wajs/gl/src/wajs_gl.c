#include <wajs_gl.h>

#include <stddef.h>
#include <time.h>

typedef struct timespec timespec;

static WajsRenderCallback s_WajsRenderCallback = NULL;

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

void wajsSetGlRenderCallback(WajsRenderCallback callback)
{
    s_WajsRenderCallback = callback;
}

void __wajsGlDraw()
{
    if (s_WajsRenderCallback != NULL) {
        s_WajsRenderCallback();
    }
}

float wajsGetFrameTime()
{
    static float wajsLastFrameTs = 0.f;
    float nw = unixTimeMiliseconds();
    float frametime = nw - wajsLastFrameTs;
    wajsLastFrameTs = nw;
    return frametime;
}
