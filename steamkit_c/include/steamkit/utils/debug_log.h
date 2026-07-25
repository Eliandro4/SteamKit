#ifndef STEAMKIT_UTILS_DEBUG_LOG_H
#define STEAMKIT_UTILS_DEBUG_LOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Debug log levels
typedef enum sk_log_level {
    SK_LOG_LEVEL_DEBUG = 0,
    SK_LOG_LEVEL_INFO = 1,
    SK_LOG_LEVEL_WARN = 2,
    SK_LOG_LEVEL_ERROR = 3,
    SK_LOG_LEVEL_OFF = 4
} sk_log_level_t;

// Initializes the debug log system
void sk_debug_log_init(const char* identifier);

// Sets the log level
void sk_debug_log_set_level(sk_log_level_t level);

// Logs a debug message
void sk_debug_log_debug(const char* category, const char* format, ...);

// Logs an info message
void sk_debug_log_info(const char* category, const char* format, ...);

// Logs a warning
void sk_debug_log_warn(const char* category, const char* format, ...);

// Logs an error
void sk_debug_log_error(const char* category, const char* format, ...);

// Shuts down the debug log system
void sk_debug_log_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_UTILS_DEBUG_LOG_H
