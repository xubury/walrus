#pragma once

#include <assert.h>
#include <core/log.h>

#define ASSERT(e) assert(e);

#define ASSERT_MSG(e, ...)      \
    if (!(e)) {                 \
        log_error(__VA_ARGS__); \
        assert(false);          \
    }

#define ARRAY_LEN(arr) sizeof(arr) / sizeof(arr[0])

#define UNUSED(x) (void)(x);
