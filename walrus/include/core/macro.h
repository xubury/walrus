#pragma once

#include <assert.h>
#include "platform.h"
#include "type.h"

#define WR_STMT_BEGIN do
#define WR_STMT_END   while (0)

#define walrus_count_of(arr) sizeof(arr) / sizeof(arr[0])

#define walrus_unused(x) (void)(x)

#define walrus_ptr_to_val(x) (u64)(x)

#define walrus_val_to_ptr(x) (void*)(u64)(x)

#if WR_COMPILER == WR_COMPILER_VC
#define WR_INLINE __forceinline
#else
#define WR_INLINE static inline __attribute((always_inline))
#endif

#if WR_COMPILER == WR_COMPILER_GCC || WR_COMPILER == WR_COMPILER_CLANG
#define walrus_assume(_condition)    \
    WR_STMT_BEGIN                    \
    {                                \
        if (!(_condition)) {         \
            __builtin_unreachable(); \
        }                            \
    }                                \
    WR_STMT_END
#define walrus_align_decl(_align, _decl) _decl __attribute__((aligned(_align)))
#define WR_ALLOW_UNUSED                  __attribute__((unused))
#define WR_FUNCTION                      __PRETTY_FUNCTION__
#define walrus_likely(_x)                __builtin_expect(!!(_x), 1)
#define walrus_unlikely(_x)              __builtin_expect(!!(_x), 0)
#define WR_NO_INLINE                     __attribute__((noinline))
#define WR_NO_RETURN                     __attribute__((noreturn))
#define WR_CONST_FUNC                    __attribute__((pure))
#define WR_UNREACHABLE                   __builtin_unreachable()

#elif WR_COMPILER == WR_COMPILER_VC
#define walrus_assume(_condition)        __assume(_condition)
#define walrus_align_decl(_align, _decl) __declspec(align(_align)) _decl
#define WR_ALLOW_UNUSED
#define WR_FUNCTION         __FUNCTION__
#define walrus_likely(_x)   (_x)
#define walrus_unlikely(_x) (_x)
#define WR_NO_INLINE        __declspec(noinline)
#define WR_NO_RETURN
#define WR_CONST_FUNC  __declspec(noalias)
#define WR_UNREACHABLE __assume(false)
#else
#error "Unknown WR_COMPILER_?"
#endif
