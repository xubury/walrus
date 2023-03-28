#pragma once

#include <assert.h>

#define ASSERT(e, ...)                \
    if (!(e)) {                       \
        assert(false);                \
        fprintf(stderr, __VA_ARGS__); \
    }

#define ARRAY_LEN(arr) sizeof(arr) / sizeof(arr[0])

