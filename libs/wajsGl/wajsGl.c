#include "wajsGl.h"

#include <stddef.h>
#include <time.h>

static WajsRenderCallback wajsRenderCallback = NULL;

static float unixTimeNanoseconds()
{
    struct timespec nw;
    clock_gettime(CLOCK_REALTIME, &nw);
    return nw.tv_nsec;
}

static float unixTimeMiliseconds()
{
    return unixTimeNanoseconds() / (1000.f * 1000.f);
}

static float unixTimeSeoncds()
{
    return unixTimeNanoseconds() / (1000.f * 1000.f * 1000.f);
}

void wajsSetGlRenderCallback(WajsRenderCallback callback)
{
    wajsRenderCallback = callback;
}

void __wajsGlDraw()
{
    if (wajsRenderCallback != NULL) {
        wajsRenderCallback();
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
