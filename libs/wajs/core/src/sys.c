#include "sys.h"

#include <math.h>
#include <time.h>

typedef struct timespec timespec;

i32 sysclock_128(u64 *sec, u64 *nano)
{
    timespec spec;
    if (timespec_get(&spec, TIME_UTC) == TIME_UTC) {
        if (sec != NULL) *sec = spec.tv_sec;
        if (nano != NULL) *nano = spec.tv_nsec;

        return 0;
    }

    return -1;
}

u64 sysclock(SysClockUnit unit)
{
    timespec spec;
    timespec_get(&spec, TIME_UTC);
    if (unit == SYS_CLOCK_UNIT_MICROSEC) {
        u64 micro = round(spec.tv_nsec * 1.0e-3);
        return micro + spec.tv_sec * 1e6;
    }
    if (unit == SYS_CLOCK_UNIT_MILLSEC) {
        u64 ms = round(spec.tv_nsec * 1.0e-6);
        return ms + spec.tv_sec * 1e3;
    }
    else if (unit == SYS_CLOCK_UNIT_SEC) {
        return spec.tv_sec;
    }
    return 0;
}
