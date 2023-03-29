#pragma once

#include "type.h"

// Function to get accurate unix time, return 0 on success, -1 otherwise
i32 nanoclock(uint64_t *sec, uint64_t *nano);

typedef enum { SYS_CLOCK_UNIT_MICROSEC = 0, SYS_CLOCK_UNIT_MILISEC = 1, SYS_CLOCK_UNIT_SEC = 2 } SysClockUnit;

// Function to get unix time in unit
u64 unitclock(SysClockUnit unit);
