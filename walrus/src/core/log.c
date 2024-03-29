#include <core/log.h>

#define MAX_CALLBACKS 32

typedef struct {
    Walrus_LogFn fn;
    void        *udata;
    i32          level;
} Walrus_Callback;

static struct {
    void            *udata;
    Walrus_LogLockFn lock;
    i32              level;
    bool             quiet;
    Walrus_Callback  callbacks[MAX_CALLBACKS];
} L;

static char const *level_strings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

#ifdef LOG_USE_COLOR
static char const *level_colors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

static void stdout_callback(Walrus_LogEvent *ev)
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

static void file_callback(Walrus_LogEvent *ev)
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

char const *walrus_log_level_string(i32 level)
{
    return level_strings[level];
}

void walrus_log_set_lock(Walrus_LogLockFn fn, void *udata)
{
    L.lock  = fn;
    L.udata = udata;
}

void walrus_log_set_level(i32 level)
{
    L.level = level;
}

void walrus_log_set_quiet(bool enable)
{
    L.quiet = enable;
}

i32 walrus_log_add_callback(Walrus_LogFn fn, void *udata, i32 level)
{
    for (i32 i = 0; i < MAX_CALLBACKS; i++) {
        if (!L.callbacks[i].fn) {
            L.callbacks[i] = (Walrus_Callback){fn, udata, level};
            return 0;
        }
    }
    return -1;
}

i32 walrus_log_add_fp(FILE *fp, i32 level)
{
    return walrus_log_add_callback(file_callback, fp, level);
}

static void init_event(Walrus_LogEvent *ev, void *udata)
{
    if (!ev->time) {
        time_t t = time(NULL);
        ev->time = localtime(&t);
    }
    ev->udata = udata;
}

void walrus_log(i32 level, char const *file, i32 line, char const *fmt, ...)
{
    Walrus_LogEvent ev = {
        .fmt   = fmt,
        .file  = file,
        .line  = line,
        .level = level,
    };

    lock();

    if (!L.quiet && level >= L.level) {
        init_event(&ev, level >= WR_LOG_ERROR ? stderr : stdout);
        va_start(ev.ap, fmt);
        stdout_callback(&ev);
        va_end(ev.ap);
    }

    for (i32 i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
        Walrus_Callback *cb = &L.callbacks[i];
        if (level >= cb->level) {
            init_event(&ev, cb->udata);
            va_start(ev.ap, fmt);
            cb->fn(&ev);
            va_end(ev.ap);
        }
    }

    unlock();
}
