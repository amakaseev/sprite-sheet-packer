/*
 * opngcore/util.h
 * Utilities.
 *
 * Copyright (C) 2010-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef OPNGCORE_UTIL_H
#define OPNGCORE_UTIL_H

#ifndef OPNGLIB_INTERNAL
#error This header, internal to opnglib, is not meant to be visible outside.
#endif

#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif


/*** Debug utilities ***/

/*
 * Strong assertion.
 * Terminates with a critical internal error (i.e. panic) on failure.
 */
#define OPNG_ASSERT(condition, message) \
    ((void)((condition) ? 0 : (opng_panic_assert(message), 0)))

/*
 * Weak assertion.
 * Displays an eye-catching error message on failure,
 * but allows the non-debug program execution to continue.
 */
#ifdef OPNG_DEBUG
#define OPNG_WEAK_ASSERT(condition, message) \
    OPNG_ASSERT(condition, message)
#else
#define OPNG_WEAK_ASSERT(condition, message) \
    ((void)((condition) ? 0 : (opng_error(NULL, "[BUG?] " message, NULL), 0)))
#endif

/*
 * Expensive assertion.
 * This is enabled only in the debug-enabled build, and disabled by default.
 */
#ifdef OPNG_DEBUG
#define OPNG_EXPENSIVE_ASSERT(condition, message) \
    OPNG_ASSERT(condition, message)
#else
#define OPNG_EXPENSIVE_ASSERT(condition, message) \
    ((void)0)
#endif


/*** Character type utilities ***/

/*
 * Tests whether a character is in the numeric [0-9] range.
 * Avoid using isdigit(), which is locale-dependent.
 */
int
opng_isdigit(int ch);

/*
 * Tests whether a character is in the alphanumeric [0-9A-Za-z] range.
 * Avoid using isalnum(), which is locale-dependent.
 */
int
opng_isalnum(int ch);

/*
 * Tests whether a character is in the alphabetic [A-Za-z] range.
 * Avoid using isalpha(), which is locale-dependent.
 */
int
opng_isalpha(int ch);

/*
 * Tests whether a character is in the uppercase alphabetic [A-Z] range.
 * Avoid using isupper(), which is locale-dependent.
 */
int
opng_isupper(int ch);

/*
 * Tests whether a character is in the lowercase alphabetic [a-z] range.
 * Avoid using islower(), which is locale-dependent.
 */
int
opng_islower(int ch);


/*** Memory utilities ***/

/*
 * Allocates memory using malloc.
 * If malloc fails, the function terminates the program with
 * a memory panic message.
 */
void *
opng_xmalloc(size_t size);

/*
 * Reallocates memory using realloc.
 * If realloc fails, the function terminates the program with
 * a memory panic message.
 */
void *
opng_xrealloc(void *ptr, size_t size);


/*** Panic handling utilities ***/

/*
 * Prints a critical message to the logger
 * and terminates the program with EX_SOFTWARE.
 */
void
opng_panic_assert(const char *message);

/*
 * Prints an out-of-memory critical message to the logger
 * and terminates the program with EX_UNAVAILABLE.
 */
void
opng_panic_memory(void);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPNGCORE_UTIL_H */
