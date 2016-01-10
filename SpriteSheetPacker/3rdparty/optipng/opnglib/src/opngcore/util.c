/*
 * opngcore/util.c
 * Utilities.
 *
 * Copyright (C) 2010-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include "opngcore.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "sysexits.h"

#define OPNGLIB_INTERNAL
#include "util.h"


/*** Character type utilities ***/

/*
 * Tests whether a character is in the numeric [0-9] range.
 */
int
opng_isdigit(int ch)
{
    return (ch >= '0' && ch <= '9');
}

/*
 * Tests whether a character is in the alphanumeric [0-9A-Za-z] range.
 */
int
opng_isalnum(int ch)
{
    return (ch >= '0' && ch <= '9') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z');
}

/*
 * Tests whether a character is in the alphabetic [A-Za-z] range.
 */
int
opng_isalpha(int ch)
{
    return (ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z');
}

/*
 * Tests whether a character is in the uppercase alphabetic [A-Z] range.
 */
int
opng_isupper(int ch)
{
    return (ch >= 'A' && ch <= 'Z');
}

/*
 * Tests whether a character is in the lowercase alphabetic [a-z] range.
 */
int
opng_islower(int ch)
{
    return (ch >= 'a' && ch <= 'z');
}


/*** Memory utilities ***/

/*
 * Allocates memory using malloc.
 */
void *
opng_xmalloc(size_t size)
{
    void *result;

    result = malloc(size);
    if (result == NULL && size != 0)
        opng_panic_memory();
    return result;
}

/*
 * Reallocates memory using realloc.
 */
void *
opng_xrealloc(void *ptr, size_t size)
{
    void *result;

    result = realloc(ptr, size);
    if (result == NULL)
        opng_panic_memory();
    return result;
}


/*** Panic handling utilities ***/

/*
 * Prints a critical message to the logger
 * and terminates the program with EX_SOFTWARE.
 */
void
opng_panic_assert(const char *message)
{
    opng_print_message(OPNG_MSG_CRITICAL, NULL, message);
    opng_flush_logging(OPNG_MSG_CRITICAL);

#ifdef OPNG_DEBUG
    abort();
#else
    fprintf(stderr,
        "The execution of this program has been terminated abnormally.\n"
        "Please submit a defect report.\n");
    exit(EX_SOFTWARE);
#endif
}

/*
 * Prints an out-of-memory critical message to the logger
 * and terminates the program with EX_UNAVAILABLE.
 */
void
opng_panic_memory(void)
{
    opng_print_message(OPNG_MSG_CRITICAL, NULL, "Out of memory");
    opng_flush_logging(OPNG_MSG_CRITICAL);

#ifdef OPNG_DEBUG
    abort();
#else
    exit(EX_UNAVAILABLE);
#endif
}
