#include <log.h>

#define MAX_CALLBACKS 32

typedef struct {
    LogFn fn;
    void *udata;
    i32   level;
} Callback;

static struct {
    void     *udata;
    LogLockFn lock;
    i32       level;
    bool      quiet;
    Callback  callbacks[MAX_CALLBACKS];
} L;

static char const *level_strings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

#ifdef LOG_USE_COLOR
static char const *level_colors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

static void stdout_callback(LogEvent *ev)
{
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
#ifdef LOG_USE_COLOR
    fprintf(ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf, level_colors[ev->level],
            level_strings[ev->level], ev->file, ev->line);
#else
    fprintf(ev->udata, "%s %-5s %s:%d: ", buf, level_strings[ev->level], ev->file, ev->line);
#endif
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}

static void file_callback(LogEvent *ev)
{
    char buf[64];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
    fprintf(ev->udata, "%s %-5s %s:%d: ", buf, level_strings[ev->level], ev->file, ev->line);
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}

static void lock(void)
{
    if (L.lock) {
        L.lock(true, L.udata);
    }
}

static void unlock(void)
{
    if (L.lock) {
        L.lock(false, L.udata);
    }
}

char const *log_level_string(i32 level)
{
    return level_strings[level];
}

void log_set_lock(LogLockFn fn, void *udata)
{
    L.lock  = fn;
    L.udata = udata;
}

void log_set_level(i32 level)
{
    L.level = level;
}

void log_set_quiet(bool enable)
{
    L.quiet = enable;
}

i32 log_add_callback(LogFn fn, void *udata, i32 level)
{
    for (i32 i = 0; i < MAX_CALLBACKS; i++) {
        if (!L.callbacks[i].fn) {
            L.callbacks[i] = (Callback){fn, udata, level};
            return 0;
        }
    }
    return -1;
}

i32 log_add_fp(FILE *fp, i32 level)
{
    return log_add_callback(file_callback, fp, level);
}

static void init_event(LogEvent *ev, void *udata)
{
    if (!ev->time) {
        time_t t = time(NULL);
        ev->time = localtime(&t);
    }
    ev->udata = udata;
}

void log_log(i32 level, char const *file, i32 line, char const *fmt, ...)
{
    LogEvent ev = {
        .fmt   = fmt,
        .file  = file,
        .line  = line,
        .level = level,
    };

    lock();

    if (!L.quiet && level >= L.level) {
        init_event(&ev, stdout);
        va_start(ev.ap, fmt);
        stdout_callback(&ev);
        va_end(ev.ap);
    }

    for (i32 i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
        Callback *cb = &L.callbacks[i];
        if (level >= cb->level) {
            init_event(&ev, cb->udata);
            va_start(ev.ap, fmt);
            cb->fn(&ev);
            va_end(ev.ap);
        }
    }

    unlock();
}
