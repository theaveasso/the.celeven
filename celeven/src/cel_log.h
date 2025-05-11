#pragma once

#include "cel.h"
#include <time.h>

/**
 * https://github.com/rxi/log.c.git
 */

typedef struct CELlog_event CELlog_event;
struct CELlog_event {
    va_list ap;
    const char *fmt;
    const char *file;
    struct tm time;
    void *data;
    int line;
    int level;
};

typedef void (*CELlog_fn)(CELlog_event *event);
typedef void (*CELlock_fn)(bool lock, void *data);

typedef enum CELlog_level
{
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
    MAX_LOG_COUNT
} CELlog_level;

const char *log_level_string(int level);
void log_set_lock(CELlock_fn fn, void *data);
void log_set_level(int level);
void log_set_quite(bool enable);
int log_add_callback(CELlog_fn fn, void *data, int level);
int log_add_fp(FILE *fp, int level);

CELAPI void cel_log(int level, const char *file, int line, const char *fmt, ...);

// clang-format off
#define CEL_TRACE(...)  cel_log(LOG_TRACE,  __FILE__, __LINE__, __VA_ARGS__)
#define CEL_DEBUG(...)  cel_log(LOG_DEBUG,  __FILE__, __LINE__, __VA_ARGS__)
#define CEL_INFO(...)   cel_log(LOG_INFO,   __FILE__, __LINE__, __VA_ARGS__)
#define CEL_WARN(...)   cel_log(LOG_WARN,   __FILE__, __LINE__, __VA_ARGS__)
#define CEL_ERROR(...)  cel_log(LOG_ERROR,  __FILE__, __LINE__, __VA_ARGS__)
#define CEL_FATAL(...)  cel_log(LOG_FATAL,  __FILE__, __LINE__, __VA_ARGS__)
// clang-format on
