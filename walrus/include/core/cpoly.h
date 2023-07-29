#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define POLY_TYPE(name)      _Poly_##name
#define POLY_VAR(var)        _poly_##var
#define POLY_INTERFACE(name) POLY_TYPE(Func_##name)
#define POLY_FUNC_PTR(name)  POLY_TYPE(FunPtr_##name)
#define POLY_PROTOTYPE(ret, name, ...)                            \
    typedef ret (*POLY_TYPE(name))(__VA_ARGS__);                  \
    static inline void *POLY_FUNC_PTR(name)(POLY_TYPE(name) func) \
    {                                                             \
        return (void *)func;                                      \
    }

#define POLY_IMPL(name, impl) [POLY_INTERFACE(name)] = POLY_FUNC_PTR(name)(impl)

typedef struct {
    char const *const type;
    void *const      *interfaces;
} POLY_TYPE(table);

#define POLY_DECLARE_DERIVED(base, derived, ctor) extern base ctor(derived *ptr);
#define POLY_DEFINE_DERIVED(base, derived, ctor, ...)                                         \
    base ctor(derived *ptr)                                                                   \
    {                                                                                         \
        static void *interfaces[POLY_TYPE(base)] = {0};                                       \
        void        *tmp[]                       = {__VA_ARGS__};                             \
        memcpy(interfaces, tmp, sizeof(tmp));                                                 \
        static POLY_TYPE(table) const tbl = {.type = #derived, .interfaces = &interfaces[0]}; \
        return (base){.POLY_VAR(self) = ptr, .POLY_VAR(table) = &tbl};                        \
    }

#define POLY_TABLE(base, ...)               \
    enum {                                  \
        __VA_ARGS__,                        \
        POLY_TYPE(base)                     \
    };                                      \
    void                   *POLY_VAR(self); \
    POLY_TYPE(table) const *POLY_VAR(table);

#define POLY_FUNC(base, name) ((POLY_TYPE(name))(base)->POLY_VAR(table)->interfaces[POLY_INTERFACE(name)])

#define poly_cast(base, derived) ((derived *)(base)->POLY_VAR(self))

#define poly_safe_cast(base, derived) \
    (strcmp((base)->POLY_VAR(table)->type, #derived) == 0 ? (derived *)(base)->POLY_VAR(self) : 0)
