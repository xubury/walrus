#pragma once

#include <assert.h>
#include <stdio.h>

#define ASSERT(e, ...)                \
    if (!(e)) {                       \
        fprintf(stderr, __VA_ARGS__); \
        assert(false);                \
    }

#define ARRAY_LEN(arr) sizeof(arr) / sizeof(arr[0])

#define UNUSED(x) (void)(x);
