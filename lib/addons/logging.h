#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdbool.h>

#ifdef __cplusplus
/* *auto-indent-off* */
extern "C" {
/* *auto-indent-on* */
#endif

void log_colors_on  (void);
void log_colors_off (void);

void log_error     (const char *format, ...) __attribute__((format (printf, 1, 2)));
void log_warn      (const char *format, ...) __attribute__((format (printf, 1, 2)));
void log_info      (const char *format, ...) __attribute__((format (printf, 1, 2)));
void log_debug     (const char *format, ...) __attribute__((format (printf, 1, 2)));

void log_extra     (const char *header, const char *format, ...) __attribute__((format (printf, 2, 3)));
void log_good      (const char *format, ...)                     __attribute__((format (printf, 1, 2)));
void log_bad       (const char *format, ...)                     __attribute__((format (printf, 1, 2)));
void log_eval      (bool good, const char *format, ...)          __attribute__((format (printf, 2, 3)));

void log_print       (const char *format, ...) __attribute__((format (printf, 1, 2)));
void log_print_debug (const char *format, ...) __attribute__((format (printf, 1, 2)));


void log_to_file (const char *filename, bool append);
void log_close_file (void);

enum log_level {
    LOG_LEVEL_INVALID = -1,
    LOG_LEVEL_DEBUG   = 0,
    LOG_LEVEL_INFO    = 1,
    LOG_LEVEL_QUIET   = 2
};

void           log_set_level (enum log_level level);
enum log_level log_get_level (void);
void           log_set_newline_mode (bool autonewline);

#ifdef __cplusplus
/* *auto-indent-off* */
}
/* *auto-indent-on* */
#endif

#endif

