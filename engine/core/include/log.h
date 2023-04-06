#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <type.h>
#include <time.h>

typedef struct {
    va_list     ap;
    char const *fmt;
    char const *file;
    struct tm  *time;
    void       *udata;
    i32         line;
    i32         level;
} log_Event;

typedef void (*log_LogFn)(log_Event *ev);
typedef void (*log_LockFn)(bool lock, void *udata);

enum {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

char const *log_level_string(i32 level);
void        log_set_lock(log_LockFn fn, void *udata);
void        log_set_level(i32 level);
void        log_set_quiet(bool enable);
i32         log_add_callback(log_LogFn fn, void *udata, i32 level);
i32         log_add_fp(FILE *fp, i32 level);

void log_log(i32 level, char const *file, i32 line, char const *fmt, ...);
