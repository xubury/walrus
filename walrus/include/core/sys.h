#pragma once

#include "type.h"

// Function to get accurate sys time, return 0 on success, -1 otherwise
i32 walrus_sysclock_128(u64 *sec, u64 *nano);

typedef enum {
    WR_SYS_CLOCK_UNIT_MICROSEC = 0,
    WR_SYS_CLOCK_UNIT_MILLSEC  = 1,
    WR_SYS_CLOCK_UNIT_SEC      = 2
} Walrus_SysClockUnit;

// Function to get sys time in unit
u64 walrus_sysclock(Walrus_SysClockUnit unit);
