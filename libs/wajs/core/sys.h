#pragma once

#include "type.h"

// Function to get accurate unix time, return 0 on success, -1 otherwise
i32 nanoclock(uint64_t *sec, uint64_t *nano);

typedef enum { SYS_CLOCK_UNIT_MS = 0, SYS_CLOCK_UNIT_SEC = 1 } SysClockUnit;

// Function to get unix time in unit
u64 unitclock(SysClockUnit unit);
