#pragma once

#include <assert.h>
#include <core/log.h>

#define walrus_assert(e) assert(e);

#define walrus_assert_msg(e, ...) \
    if (!(e)) {                   \
        walrus_error(__VA_ARGS__);   \
        assert(false);            \
    }

#define walrus_array_len(arr) sizeof(arr) / sizeof(arr[0])

#define walrus_unused(x) (void)(x);
