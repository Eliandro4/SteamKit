#include "steamkit/utils/debug_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

static sk_log_level_t g_log_level = SK_LOG_LEVEL_INFO;
static FILE* g_log_file = NULL;

void sk_debug_log_init(const char* identifier) {
    (void)identifier;
    g_log_file = stderr;
}

void sk_debug_log_set_level(sk_log_level_t level) {
    g_log_level = level;
}

static void log_message(sk_log_level_t level, const char* category, const char* format, va_list args) {
    if (level < g_log_level) return;
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    const char* prefix = "";
    switch (level) {
        case SK_LOG_LEVEL_DEBUG: prefix = "[DEBUG]"; break;
        case SK_LOG_LEVEL_INFO:  prefix = "[INFO]"; break;
        case SK_LOG_LEVEL_WARN:  prefix = "[WARN]"; break;
        case SK_LOG_LEVEL_ERROR: prefix = "[ERROR]"; break;
        default: return;
    }
    
    FILE* out = g_log_file ? g_log_file : stderr;
    fprintf(out, "%s %s [%s] ", timestamp, prefix, category);
    vfprintf(out, format, args);
    fprintf(out, "\n");
    fflush(out);
}

void sk_debug_log_debug(const char* category, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(SK_LOG_LEVEL_DEBUG, category, format, args);
    va_end(args);
}

void sk_debug_log_info(const char* category, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(SK_LOG_LEVEL_INFO, category, format, args);
    va_end(args);
}

void sk_debug_log_warn(const char* category, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(SK_LOG_LEVEL_WARN, category, format, args);
    va_end(args);
}

void sk_debug_log_error(const char* category, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(SK_LOG_LEVEL_ERROR, category, format, args);
    va_end(args);
}

void sk_debug_log_shutdown(void) {
    if (g_log_file && g_log_file != stderr && g_log_file != stdout) {
        fclose(g_log_file);
    }
    g_log_file = NULL;
}
