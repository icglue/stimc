/*
 *  stimc is a lightweight verilog-vpi wrapper for stimuli generation.
 *  Copyright (C) 2019-2021  Andreas Dixius, Felix Neumärker
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "logging.h"

#include <vpi_user.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static const char ANSI_COLOR_RED[]     = "\x1b[31m";
static const char ANSI_COLOR_GREEN[]   = "\x1b[32m";
static const char ANSI_COLOR_YELLOW[]  = "\x1b[33m";
static const char ANSI_COLOR_BLUE[]    = "\x1b[34m";
static const char ANSI_COLOR_MAGENTA[] = "\x1b[35m";
static const char ANSI_COLOR_CYAN[]    = "\x1b[36m";
static const char ANSI_RESET[]         = "\x1b[0m";
static const char ANSI_BOLD[]          = "\x1b[1m";
static const char ANSI_NOTHING[]       = "";

#ifdef LOGGING_ENABLE_COLORS_DEFAULT
static const char *ansi_color_red     = ANSI_COLOR_RED;
static const char *ansi_color_green   = ANSI_COLOR_GREEN;
static const char *ansi_color_yellow  = ANSI_COLOR_YELLOW;
static const char *ansi_color_blue    = ANSI_COLOR_BLUE;
static const char *ansi_color_magenta = ANSI_COLOR_MAGENTA;
static const char *ansi_color_cyan    = ANSI_COLOR_CYAN;
static const char *ansi_reset         = ANSI_RESET;
static const char *ansi_bold          = ANSI_BOLD;
#else
static const char *ansi_color_red     = ANSI_NOTHING;
static const char *ansi_color_green   = ANSI_NOTHING;
static const char *ansi_color_yellow  = ANSI_NOTHING;
static const char *ansi_color_blue    = ANSI_NOTHING;
static const char *ansi_color_magenta = ANSI_NOTHING;
static const char *ansi_color_cyan    = ANSI_NOTHING;
static const char *ansi_reset         = ANSI_NOTHING;
static const char *ansi_bold          = ANSI_NOTHING;
#endif

/* logging to file */
static FILE *logfile = NULL;

/* log level */
static enum log_level current_level = LOG_LEVEL_INFO;

#define LOG_HEADER_SIZE 128
static char         log_header_file[LOG_HEADER_SIZE];
static char         log_header_out[LOG_HEADER_SIZE];
static size_t       log_header_file_pos = 0;
static size_t       log_header_out_pos  = 0;
static const size_t log_header_size     = LOG_HEADER_SIZE;

/* modes */
enum log_mode {
    LOG_MODE_INIT,
    LOG_MODE_ERROR,
    LOG_MODE_WARNING,
    LOG_MODE_INFO,
    LOG_MODE_EXTRA,
    LOG_MODE_DEBUG,
    LOG_MODE_GOOD,
    LOG_MODE_BAD,
    LOG_MODE_PRINT
};

static enum log_mode current_mode    = LOG_MODE_INIT;
static enum log_mode last_mode       = LOG_MODE_INIT;
static bool          log_autonewline = true;

static void log_base_reset (enum log_mode mode)
{
    log_header_file[0]  = '\0';
    log_header_out[0]   = '\0';
    log_header_file_pos = 0;
    log_header_out_pos  = 0;
    current_mode        = mode;
}

static void log_base_modify (const char *mod)
{
    int print_size = snprintf (&(log_header_out[log_header_out_pos]), log_header_size - log_header_out_pos, "%s", mod);

    log_header_out_pos += (size_t)print_size;

    assert (log_header_out_pos < log_header_size);
}

static void log_base_header (const char *header)
{
    int print_size = snprintf (&(log_header_out[log_header_out_pos]), log_header_size - log_header_out_pos, "%-10s", header);

    log_header_out_pos += (size_t)print_size;

    assert (log_header_out_pos < log_header_size);

    print_size = snprintf (&(log_header_file[log_header_file_pos]), log_header_size - log_header_file_pos, "%-10s", header);

    log_header_file_pos += (size_t)print_size;

    assert (log_header_file_pos < log_header_size);
}


static void log_base_vprintf (const char *format, va_list arg_list)
{
    #define LOG_BUFF_SIZE 4096
    const size_t log_buff_size = LOG_BUFF_SIZE;
    static char  log_buff[LOG_BUFF_SIZE];
    #undef LOG_BUFF_SIZE

    char *log_string = &(log_buff[0]);

    /* copy va_list in case a second attempt is necessary */
    va_list arg_list_temp;
    va_copy (arg_list_temp, arg_list);
    size_t would_have_written = (size_t)vsnprintf (log_string, log_buff_size, format, arg_list_temp);
    va_end (arg_list_temp);

    size_t size_target = would_have_written + 1;

    if (size_target > log_buff_size) {
        log_string = malloc (size_target);
        assert (log_string);

        would_have_written = (size_t)vsnprintf (log_string, size_target, format, arg_list);

        assert (would_have_written < size_target);
    }

    char *line = log_string;

    while (line != NULL) {
        char *line_next = line;
        bool  newline   = false;
        while ((*line_next != '\0') && (*line_next != '\n')) line_next++;
        if (*line_next == '\n') {
            *line_next = '\0';
            line_next++;
            newline = true;
        } else {
            line_next = NULL;
        }

        if (log_autonewline) {
            /* always add header and newline (for newline and at end of message) */
            if (logfile != NULL) {
                fprintf (logfile, "%s%s\n", log_header_file, line);
            }

            vpi_printf ("%s%s%s\n", log_header_out, line, ansi_reset);
        } else if ((*line != '\0') || (newline)) {
            /* header - only after newline (last_mode == INIT) or log-type change */
            if (current_mode != last_mode) {
                if (logfile != NULL) {
                    fprintf (logfile, "%s", log_header_file);
                }

                vpi_printf ("%s%s", ansi_reset, log_header_out);
                last_mode = current_mode;
            }

            /* text */
            if (logfile != NULL) {
                fprintf (logfile, "%s", line);
            }

            vpi_printf ("%s", line);

            /* newline ? */
            if (newline) {
                if (logfile != NULL) {
                    fprintf (logfile, "\n");
                }

                vpi_printf ("%s\n", ansi_reset);
                last_mode = LOG_MODE_INIT;
            }
        }

        line = line_next;
    }

    if (log_string != &(log_buff[0])) {
        free (log_string);
    }
}


void log_to_file (const char *filename, bool append)
{
    log_close_file ();
    logfile = fopen (filename, (append ? "a" : "w"));
    if (logfile == NULL) {
        log_error ("could not open file %s for logging", filename);
    }
}

void log_close_file (void)
{
    if (logfile != NULL) {
        fclose (logfile);
        logfile = NULL;
    }
}

void log_set_level (enum log_level level)
{
    current_level = level;
}

enum log_level log_get_level (void)
{
    return current_level;
}

/* colors */
void log_colors_on (void)
{
    ansi_color_red     = ANSI_COLOR_RED;
    ansi_color_green   = ANSI_COLOR_GREEN;
    ansi_color_yellow  = ANSI_COLOR_YELLOW;
    ansi_color_blue    = ANSI_COLOR_BLUE;
    ansi_color_magenta = ANSI_COLOR_MAGENTA;
    ansi_color_cyan    = ANSI_COLOR_CYAN;
    ansi_reset         = ANSI_RESET;
    ansi_bold          = ANSI_BOLD;
}

void log_colors_off (void)
{
    ansi_color_red     = ANSI_NOTHING;
    ansi_color_green   = ANSI_NOTHING;
    ansi_color_yellow  = ANSI_NOTHING;
    ansi_color_blue    = ANSI_NOTHING;
    ansi_color_magenta = ANSI_NOTHING;
    ansi_color_cyan    = ANSI_NOTHING;
    ansi_reset         = ANSI_NOTHING;
    ansi_bold          = ANSI_NOTHING;
}

/* logging */

void log_set_newline_mode (bool autonewline)
{
    log_autonewline = autonewline;
}

/* log functions */
void log_error (const char *format, ...)
{
    log_base_reset  (LOG_MODE_ERROR);
    log_base_modify (ansi_color_red);
    log_base_modify (ansi_bold);
    log_base_header ("ERROR:");
    log_base_modify (ansi_reset);
    log_base_modify (ansi_color_red);
    va_list argptr;

    va_start (argptr, format);
    log_base_vprintf (format, argptr);
    va_end (argptr);
}


void log_warn (const char *format, ...)
{
    log_base_reset  (LOG_MODE_WARNING);
    log_base_modify (ansi_color_yellow);
    log_base_modify (ansi_bold);
    log_base_header ("WARNING:");
    log_base_modify (ansi_reset);
    log_base_modify (ansi_color_yellow);
    va_list argptr;

    va_start (argptr, format);
    log_base_vprintf (format, argptr);
    va_end (argptr);
}

void log_info (const char *format, ...)
{
    if (current_level > LOG_LEVEL_INFO) return;
    log_base_reset  (LOG_MODE_INFO);
    log_base_modify (ansi_bold);
    log_base_header ("INFO:");
    log_base_modify (ansi_reset);
    va_list argptr;

    va_start (argptr, format);
    log_base_vprintf (format, argptr);
    va_end (argptr);
}

void log_debug (const char *format, ...)
{
    if (current_level > LOG_LEVEL_DEBUG) return;
    log_base_reset  (LOG_MODE_DEBUG);
    log_base_modify (ansi_reset);
    log_base_header ("DEBUG:");
    va_list argptr;

    va_start (argptr, format);
    log_base_vprintf (format, argptr);
    va_end (argptr);
}

void log_extra (const char *header, const char *format, ...)
{
    log_base_reset  (LOG_MODE_EXTRA);
    log_base_modify (ansi_color_blue);
    log_base_modify (ansi_bold);
    log_base_header (header);
    log_base_modify (ansi_reset);
    va_list argptr;

    va_start (argptr, format);
    log_base_vprintf (format, argptr);
    va_end (argptr);
}

void log_good (const char *format, ...)
{
    if (current_level > LOG_LEVEL_INFO) return;
    log_base_reset  (LOG_MODE_GOOD);
    log_base_modify (ansi_color_green);
    log_base_modify (ansi_bold);
    log_base_header ("INFO:");
    va_list argptr;

    va_start (argptr, format);
    log_base_vprintf (format, argptr);
    va_end (argptr);
}

void log_bad (const char *format, ...)
{
    log_base_reset  (LOG_MODE_BAD);
    log_base_modify (ansi_color_red);
    log_base_modify (ansi_bold);
    log_base_header ("WARNING:");
    va_list argptr;

    va_start (argptr, format);
    log_base_vprintf (format, argptr);
    va_end (argptr);
}

void log_eval (bool good, const char *format, ...)
{
    if (good) {
        if (current_level > LOG_LEVEL_INFO) return;
        log_base_reset  (LOG_MODE_GOOD);
        log_base_modify (ansi_color_green);
        log_base_modify (ansi_bold);
        log_base_header ("INFO:");
    } else {
        log_base_reset  (LOG_MODE_BAD);
        log_base_modify (ansi_color_red);
        log_base_modify (ansi_bold);
        log_base_header ("WARNING:");
    }
    va_list argptr;

    va_start (argptr, format);
    log_base_vprintf (format, argptr);
    va_end (argptr);
}

void log_print (const char *format, ...)
{
    if (current_level > LOG_LEVEL_INFO) return;
    log_base_reset  (LOG_MODE_PRINT);
    log_base_modify (ansi_reset);
    va_list argptr;

    va_start (argptr, format);
    log_base_vprintf (format, argptr);
    va_end (argptr);
}

void log_print_debug (const char *format, ...)
{
    if (current_level > LOG_LEVEL_DEBUG) return;
    log_base_reset  (LOG_MODE_PRINT);
    log_base_modify (ansi_reset);
    va_list argptr;

    va_start (argptr, format);
    log_base_vprintf (format, argptr);
    va_end (argptr);
}

