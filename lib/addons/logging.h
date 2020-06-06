#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdbool.h>

#ifdef __cplusplus
/* *auto-indent-off* */
extern "C" {
/* *auto-indent-on* */
#endif

/**
 * @brief Log error message. Will always be printed.
 *
 * @param format printf style format string.
 *
 * Depending on newline mode (@ref log_set_newline_mode) a final
 * newline will be added automatically.
 */
void log_error (const char *format, ...) __attribute__((format (printf, 1, 2)));

/**
 * @brief Log warning message. Will always be printed.
 *
 * @param format printf style format string.
 *
 * Depending on newline mode (@ref log_set_newline_mode) a final
 * newline will be added automatically.
 */
void log_warn (const char *format, ...) __attribute__((format (printf, 1, 2)));

/**
 * @brief Log info message. Will be suppressed in @ref LOG_LEVEL_QUIET.
 *
 * @param format printf style format string.
 *
 * Depending on newline mode (@ref log_set_newline_mode) a final
 * newline will be added automatically.
 */
void log_info (const char *format, ...) __attribute__((format (printf, 1, 2)));

/**
 * @brief Log debug message. Will only be printed in @ref LOG_LEVEL_DEBUG.
 *
 * @param format printf style format string.
 *
 * Depending on newline mode (@ref log_set_newline_mode) a final
 * newline will be added automatically.
 */
void log_debug (const char *format, ...) __attribute__((format (printf, 1, 2)));

/**
 * @brief Log custom message. Will always be printed.
 *
 * @param header Custom string prefix (should be no more than 8 characters).
 * @param format printf style format string.
 *
 * Depending on newline mode (@ref log_set_newline_mode) a final
 * newline will be added automatically.
 */
void log_extra (const char *header, const char *format, ...) __attribute__((format (printf, 2, 3)));

/**
 * @brief Log "success" message. Will be suppressed in @ref LOG_LEVEL_QUIET.
 *
 * @param format printf style format string.
 *
 * Depending on newline mode (@ref log_set_newline_mode) a final
 * newline will be added automatically.
 */
void log_good (const char *format, ...) __attribute__((format (printf, 1, 2)));

/**
 * @brief Log "failure" message. Will always be printed.
 *
 * @param format printf style format string.
 *
 * Depending on newline mode (@ref log_set_newline_mode) a final
 * newline will be added automatically.
 */
void log_bad (const char *format, ...) __attribute__((format (printf, 1, 2)));

/**
 * @brief Log "success"/"failure" message based on first argument.
 *
 * @param good If true, behaves similar to @ref log_good, otherwise similar to @ref log_bad.
 * @param format printf style format string.
 */
void log_eval (bool good, const char *format, ...) __attribute__((format (printf, 2, 3)));

/**
 * @brief Log without header. Will be suppressed in @ref LOG_LEVEL_QUIET.
 *
 * @param format printf style format string.
 *
 * Depending on newline mode (@ref log_set_newline_mode) a final
 * newline will be added automatically.
 */
void log_print (const char *format, ...) __attribute__((format (printf, 1, 2)));

/**
 * @brief Log without header. Will only be printed in @ref LOG_LEVEL_DEBUG.
 *
 * @param format printf style format string.
 *
 * Depending on newline mode (@ref log_set_newline_mode) a final
 * newline will be added automatically.
 */
void log_print_debug (const char *format, ...) __attribute__((format (printf, 1, 2)));


/**
 * @brief Log to file specified in addition to stdout.
 *
 * @param filename Path to file to open for logging.
 * @param append Open file in append mode.
 */
void log_to_file (const char *filename, bool append);

/**
 * @brief Close logfile opened with @ref log_to_file.
 */
void log_close_file (void);

/**
 * @brief Log levels.
 */
enum log_level {
    LOG_LEVEL_DEBUG = 0, /**< @brief Debug level (print all log messages). */
    LOG_LEVEL_INFO  = 1, /**< @brief Info level (disables debug log messages). */
    LOG_LEVEL_QUIET = 2  /**< @brief Quiet level (print only warnings and errors). */
};

/**
 * @brief Set log level.
 *
 * @param level New log level.
 *
 * Default level is @ref LOG_LEVEL_INFO.
 */
void log_set_level (enum log_level level);

/**
 * @brief Get the current log level.
 *
 * @return Current log level.
 */
enum log_level log_get_level (void);

/**
 * @brief Log printout newline-mode.
 *
 * @param autonewline Enable autonewline.
 *
 * If autonewline is enabled (default), logging functions will always implicitly
 * add a newline at the end of every log message and start a new log message
 * with the logging header of the given log type.
 *
 * If autonewline is disabled, logging will print the message as it is
 * and only print the header between different logging types or after an
 * explicit newline.
 */
void log_set_newline_mode (bool autonewline);

/**
 * @brief Enable ANSI color codes for log heders.
 */
void log_colors_on (void);

/**
 * @brief Disable ANSI color codes for log heders.
 */
void log_colors_off (void);

#ifdef __cplusplus
/* *auto-indent-off* */
}
/* *auto-indent-on* */
#endif

#endif

