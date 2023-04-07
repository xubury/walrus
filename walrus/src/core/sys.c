#include <core/sys.h>

#include <math.h>
#include <time.h>

typedef struct timespec timespec;

i32 walrus_sysclock_128(u64 *sec, u64 *nano)
{
    timespec spec;
    if (timespec_get(&spec, TIME_UTC) == TIME_UTC) {
        if (sec != NULL) *sec = spec.tv_sec;
        if (nano != NULL) *nano = spec.tv_nsec;

        return 0;
    }

    return -1;
}

u64 walrus_sysclock(Walrus_SysClockUnit unit)
{
    timespec spec;
    if (timespec_get(&spec, TIME_UTC) == TIME_UTC) {
        if (unit == WR_SYS_CLOCK_UNIT_MICROSEC) {
            u64 micro = round(spec.tv_nsec * 1.0e-3);
            return micro + spec.tv_sec * 1e6;
        }
        if (unit == WR_SYS_CLOCK_UNIT_MILLSEC) {
            u64 ms = round(spec.tv_nsec * 1.0e-6);
            return ms + spec.tv_sec * 1e3;
        }
        else if (unit == WR_SYS_CLOCK_UNIT_SEC) {
            return spec.tv_sec;
        }
    }
    return 0;
}
