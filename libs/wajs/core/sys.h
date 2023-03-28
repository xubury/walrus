#pragma once

#include "type.h"

// Function to get unix time, return 0 on success, -1 otherwise
i32 unixclock(uint64_t *sec, uint64_t *nano);

// Helper to get unix clock in miliseconds
u64 clockms();

// Helper to get unix clock in seconds
u64 clocksec();
