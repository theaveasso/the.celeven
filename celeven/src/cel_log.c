#include "cel_log.h"

#include <stdarg.h>
#include <time.h>

#define CEL_LOG_MAX_CALLBACKS 32

typedef struct CELlog_callback CELlog_callback;
struct CELlog_callback {
    CELlog_fn fn;
    void *data;
    int level;
};

GlobalVariable struct {
    void *data;
    CELlock_fn lock;
    int level;
    bool quiet;
    CELlog_callback callbacks[CEL_LOG_MAX_CALLBACKS];
} L;

GlobalVariable const char *level_strings[] = {"CELtrace", "CELdebug", "CELinfo", "CELwarn", "CELerror", "CELfatal"};

#if defined(CEL_LOG_WITH_COLOR)
GlobalVariable const char *level_colors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

Internal void lock() {
    if (L.lock) { L.lock(true, L.data); }
}

Internal void unlock() {
    if (L.lock) { L.lock(false, L.data); }
}

Internal inline const char *cel_log_level_string(int level) {
    if (level < 0 || level >= MAX_LOG_COUNT) return "UNKNOWN";
    return level_strings[level];
}

Internal inline const char *cel_log_level_color(int level) {
#if defined(CEL_LOG_WITH_COLOR)
    if (level < 0 || level >= MAX_LOG_COUNT) return "\x1b[0m";
    return level_colors[level];
#else
    return "";
#endif
}

Internal void stdout_callback(CELlog_event *event) {
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &event->time);

#if defined(CEL_LOG_WITH_COLOR)
    fprintf(event->data, "%s %s%-7s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf, cel_log_level_color(event->level), cel_log_level_string(event->level), event->file, event->line);
#else
    fprintf(event->data, "%s %-7s %s:%d: ", buf, cel_log_level_string(event->level), event->file, event->line);
#endif

    vfprintf(event->data, event->fmt, event->ap);
    fprintf(event->data, "\n");
    fflush(event->data);
}

Internal void file_callback(CELlog_event *event) {
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &event->time);
    fprintf(event->data, "%s %-5s %s:%d: ", buf, cel_log_level_string(event->level), event->file, event->line);
    vfprintf(event->data, event->fmt, event->ap);
    fprintf(event->data, "\n");
    fflush(event->data);
}

Internal void event_init(CELlog_event *event, void *data) {
    time_t t = time(NULL);
#if defined(_WIN32)
    localtime_s(&event->time, &t);
#else
    localtime_r(&t, &event->time);
#endif
    event->data = data;
}

void log_set_lock(CELlock_fn fn, void *data) {
    L.lock = fn;
    L.data = data;
}

void log_set_level(int level) {
    L.level = level;
}

void log_set_quite(bool enable) {
    L.quiet = enable;
}

int log_add_callback(CELlog_fn fn, void *data, int level) {
    for (int i = 0; i < CEL_LOG_MAX_CALLBACKS; ++i)
    {
        if (!L.callbacks[i].fn)
        {
            L.callbacks[i] = (CELlog_callback){fn, data, level};
            return 0;
        }
    }
    return -1;
}

int log_add_fp(FILE *fp, int level) {
    return log_add_callback(file_callback, fp, level);
}

void cel_log(int level, const char *file, int line, const char *fmt, ...) {
    CELlog_event event = (CELlog_event){.fmt = fmt, .file = file, .line = line, .level = level};

    lock();

    if (!L.quiet && level >= L.level)
    {
        event_init(&event, stderr);
        va_start(event.ap, fmt);
        stdout_callback(&event);
        va_end(event.ap);
    }

    for (int i = 0; i < CEL_LOG_MAX_CALLBACKS && L.callbacks[i].fn; ++i)
    {
        CELlog_callback *cb = &L.callbacks[i];
        if (level >= cb->level)
        {
            event_init(&event, cb->data);
            va_start(event.ap, fmt);
            cb->fn(&event);
            va_end(event.ap);
        }
    }

    unlock();
}
