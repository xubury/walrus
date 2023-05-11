#pragma once

#include <assert.h>
#include "log.h"

#define walrus_assert(e) assert(e);

#define walrus_assert_msg(e, ...)  \
    if (!(e)) {                    \
        walrus_error(__VA_ARGS__); \
        assert(false);             \
    }
