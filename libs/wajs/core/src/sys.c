#include "sys.h"

#include <math.h>
#include <time.h>

typedef struct timespec timespec;

i32 unixclock(uint64_t *sec, uint64_t *nano)
{
    timespec spec;
    if (clock_gettime(CLOCK_REALTIME, &spec) == 0) {
        if (sec != NULL) *sec = spec.tv_sec;
        if (nano != NULL) *nano = spec.tv_nsec;

        return 0;
    }

    return -1;
}

u64 clocksec()
{
    timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec;
}

u64 clockms()
{
    timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

    time_t s  = spec.tv_sec;
    u64    ms = round(spec.tv_nsec / 1.0e6);
    return ms + s * 1e3;
}
