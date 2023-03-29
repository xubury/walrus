#pragma once

#include "type.h"

// Function to get accurate sys time, return 0 on success, -1 otherwise
i32 sysclock_128(u64 *sec, u64 *nano);

typedef enum {
    SYS_CLOCK_UNIT_MICROSEC = 0,
    SYS_CLOCK_UNIT_MILLSEC  = 1,
    SYS_CLOCK_UNIT_SEC      = 2
} SysClockUnit;

// Function to get sys time in unit
u64 sysclock(SysClockUnit unit);
