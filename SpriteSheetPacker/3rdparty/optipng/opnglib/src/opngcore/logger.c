/*
 * opngcore/logger.c
 * Logging utilities.
 *
 * Copyright (C) 2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include "opngcore.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sysexits.h"


static const char *logging_program_name = NULL;
static unsigned int logging_level = OPNG_MSG_DEFAULT;
static int logging_format = OPNG_MSGFMT_DEFAULT;
static int logging_start_of_line = 1;


/*
 * Initializes the logger.
 */
void
opng_set_logging_name(const char *program_name)
{
    logging_program_name = program_name;
}

/*
 * Sets the logging severity level.
 */
void
opng_set_logging_level(unsigned int level)
{
    logging_level = level;
}

/*
 * Sets the logging format.
 */
void
opng_set_logging_format(int format)
{
    logging_format =
        (format >= OPNG_MSGFMT_RAW && format <= OPNG_MSGFMT_FANCY) ?
        format : OPNG_MSGFMT_DEFAULT;
}

/*
 * Flushes the logger at the given severity level.
 */
int
opng_flush_logging(unsigned int level)
{
    if (logging_level > level || logging_level == OPNG_MSG_OFF)
        return 0;

    return fflush(stderr);
}

/*
 * Prints a message of a given level, concerning a given file name,
 * using the current logging message format, to the logger.
 */
int
opng_print_message(unsigned int level, const char *fname, const char *message)
{
    const char *wrap_str;
    const char *name_str;
    const char *name_sep_str;
    const char *level_str;

    if (logging_level > level || logging_level == OPNG_MSG_OFF)
        return 0;

    /* Prepend '\n' if the previous line is unfinished, if applicable. */
    wrap_str = "";
    if (logging_format != OPNG_MSGFMT_RAW)
    {
        if (!logging_start_of_line)
            wrap_str = "\n";
    }

    /* Add the file name or program name, if applicable. */
    name_str = name_sep_str = "";
    if (logging_format == OPNG_MSGFMT_UNIX)
    {
        if (fname != NULL)
        {
            name_str = fname;
            name_sep_str = ": ";
        }
        else if (logging_program_name != NULL)
        {
            name_str = logging_program_name;
            name_sep_str = ": ";
        }
    }

    /* Add the severity level name. */
    level_str = "";
    if (logging_format != OPNG_MSGFMT_RAW)
    {
        switch (level)
        {
        case OPNG_MSG_INFO:
            level_str = (logging_format == OPNG_MSGFMT_FANCY) ?
                "** " : "";
            break;
        case OPNG_MSG_WARNING:
            level_str = (logging_format == OPNG_MSGFMT_FANCY) ?
                "** Warning: " : "warning: ";
            break;
        case OPNG_MSG_ERROR:
            level_str = (logging_format == OPNG_MSGFMT_FANCY) ?
                "** Error: " : "error: ";
            break;
        case OPNG_MSG_CRITICAL:
            level_str = (logging_format == OPNG_MSGFMT_FANCY) ?
                "** CRITICAL ERROR: " : "CRITICAL ERROR: ";
            break;
        }
    }

    /* Print the message. */
    fprintf(stderr, "%s%s%s%s%s\n",
            wrap_str, name_str, name_sep_str, level_str, message);
    logging_start_of_line = 1;
    return ferror(stderr) ? -1 : 1;
}

/*
 * Prints a vprintf-formatted message to the logger.
 */
static int
opng_vprintf_impl(const char *format, va_list arg_ptr)
{
    int last_char;

    if (format[0] == 0)
        return 0;

    last_char = format[strlen(format) - 1];
    logging_start_of_line = (last_char == '\n') || (last_char == '\r');
    return vfprintf(stderr, format, arg_ptr);
}

/*
 * Prints a printf-formatted debug message (level = OPNG_MSG_DEBUG)
 * to the logger.
 */
int
opng_debugf(const char *format, ...)
{
    va_list arg_ptr;
    int result;

    if (logging_level > OPNG_MSG_DEBUG)
        return 0;

    va_start(arg_ptr, format);
    result = opng_vprintf_impl(format, arg_ptr);
    va_end(arg_ptr);
    return result;
}

/*
 * Prints a printf-formatted informational message (level = OPNG_MSG_INFO)
 * to the logger.
 */
int
opng_printf(const char *format, ...)
{
    va_list arg_ptr;
    int result;

    if (logging_level > OPNG_MSG_INFO)
        return 0;

    va_start(arg_ptr, format);
    result = opng_vprintf_impl(format, arg_ptr);
    va_end(arg_ptr);
    return result;
}

/*
 * Prints a warning message (level = OPNG_MSG_WARNING) to the logger.
 */
int
opng_warning(const char *fname, const char *message)
{
    return opng_print_message(OPNG_MSG_WARNING, fname, message);
}

/*
 * Prints an error message (level = OPNG_MSG_ERROR), optionally accompanied
 * by a submessage, to the logger.
 */
int
opng_error(const char *fname, const char *message, const char *submessage)
{
    int result = opng_print_message(OPNG_MSG_ERROR, fname, message);
    if (submessage != NULL)
        opng_print_message(OPNG_MSG_INFO, NULL, submessage);
    return result;
}
