#pragma once

#include <assert.h>
#include "platform.h"
#include "type.h"

#define WR_STMT_BEGIN do
#define WR_STMT_END   while (0)

#define walrus_array_len(arr) sizeof(arr) / sizeof(arr[0])

#define walrus_unused(x) (void)(x)

#define walrus_ptr_to_val(x) (u64)(x)

#define walrus_to_ptr(x) (void*)(u64)(x)

#if WR_COMPILER == WR_COMPILER_VC
#define WR_INLINE __forceinline
#else
#define WR_INLINE static inline __attribute((always_inline))
#endif
