#include "sys.h"

#include <math.h>
#include <time.h>

typedef struct timespec timespec;

i32 sysclock_128(u64 *sec, u64 *nano)
{
    timespec spec;
    if (clock_gettime(CLOCK_REALTIME, &spec) == 0) {
        if (sec != NULL) *sec = spec.tv_sec;
        if (nano != NULL) *nano = spec.tv_nsec;

        return 0;
    }

    return -1;
}

u64 sysclock(SysClockUnit unit)
{
    timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    if (unit == SYS_CLOCK_UNIT_MICROSEC) {
        u64 micro = round(spec.tv_nsec / 1.0e3);
        return micro + spec.tv_sec * 1e6;
    }
    if (unit == SYS_CLOCK_UNIT_MILLSEC) {
        u64 ms = round(spec.tv_nsec / 1.0e6);
        return ms + spec.tv_sec * 1e3;
    }
    else if (unit == SYS_CLOCK_UNIT_SEC) {
        return spec.tv_sec;
    }
    return 0;
}
