#pragma once

#include <core/type.h>

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

typedef struct {
    va_list     ap;
    char const *fmt;
    char const *file;
    struct tm  *time;
    void       *udata;
    i32         line;
    i32         level;
} Walrus_LogEvent;

typedef void (*Walrus_LogFn)(Walrus_LogEvent *ev);
typedef void (*Walrus_LogLockFn)(bool lock, void *udata);

enum {
    WR_LOG_TRACE,
    WR_LOG_DEBUG,
    WR_LOG_INFO,
    WR_LOG_WARN,
    WR_LOG_ERROR,
    WR_LOG_FATAL
};

#define walrus_trace(...) walrus_log(WR_LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define walrus_debug(...) walrus_log(WR_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define walrus_info(...)  walrus_log(WR_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define walrus_warn(...)  walrus_log(WR_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define walrus_error(...) walrus_log(WR_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define walrus_fatal(...) walrus_log(WR_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

char const *walrus_level_string(i32 level);
void        walrus_set_lock(Walrus_LogLockFn fn, void *udata);
void        walrus_set_level(i32 level);
void        walrus_set_quiet(bool enable);
i32         walrus_add_callback(Walrus_LogFn fn, void *udata, i32 level);
i32         walrus_add_fp(FILE *fp, i32 level);

void walrus_log(i32 level, char const *file, i32 line, char const *fmt, ...);
