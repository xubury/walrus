#pragma once

#include <assert.h>
#include "platform.h"
#include "type.h"
#include "log.h"

#define WR_STMT_BEGIN do
#define WR_STMT_END   while (0)
// clang-format on

#define walrus_assert(e) assert(e);

#define walrus_assert_msg(e, ...)  \
    if (!(e)) {                    \
        walrus_error(__VA_ARGS__); \
        assert(false);             \
    }

#define walrus_array_len(arr) sizeof(arr) / sizeof(arr[0])

#define walrus_unused(x) (void)(x)

#define walrus_u32_to_ptr(x) (void*)(u64)(x)

#define walrus_ptr_to_u32(x) (u32)(u64)(x)

#if WR_COMPILER == WR_COMPILER_VC
#define WR_INLINE __forceinline
#else
#define WR_INLINE static inline __attribute((always_inline))
#endif