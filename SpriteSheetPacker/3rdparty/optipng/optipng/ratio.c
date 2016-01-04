/*
 * ratio.c
 * Exact rational numbers.
 *
 * Copyright (C) 2003-2014 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include "ratio.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef OPNG_LLONG_T_DEFINED
typedef opng_llong_t opng_longest_impl_t;
typedef opng_ullong_t opng_ulongest_impl_t;
#define OPNG_LONGEST_IMPL_FORMAT OPNG_LLONG_FORMAT
#else
typedef long opng_longest_impl_t;
typedef unsigned long opng_longest_impl_t;
#define OPNG_LONGEST_IMPL_FORMAT "l"
#endif


/*
 * Writes formatted output to a memory buffer.
 * This is a wrapper to [v]snprintf which avoids well-known defects
 * occurring in some of the underlying snprintf implementations.
 * The function returns the number of characters written, excluding the
 * null-termination character, if the buffer size is large enough, or -1
 * otherwise. (Unlike the proper snprintf, this function does not return
 * a number larger than zero if the buffer size is too small.)
 */
static int
opng_snprintf_impl(char *buffer, size_t buffer_size, const char *format, ...)
{

#if defined _WIN32 || defined __WIN32__ || defined _WIN64 || defined __WIN64__
#define OPNG_VSNPRINTF _vsnprintf
#else
#define OPNG_VSNPRINTF vsnprintf
#endif

    va_list arg_ptr;
    int result;

    va_start(arg_ptr, format);
    result = OPNG_VSNPRINTF(buffer, buffer_size, format, arg_ptr);
    va_end(arg_ptr);

    if (result < 0 || (size_t)result >= buffer_size)
    {
        /* Guard against broken [v]snprintf implementations. */
        if (buffer_size > 0)
            buffer[buffer_size - 1] = '\0';
        return -1;
    }
    return result;

#undef OPNG_VSNPRINTF

}

/*
 * Writes a decomposed rational value to a memory buffer.
 * This is the base implementation used internally by the the other
 * ratio-to-string conversion functions.
 */
static int
opng_sprint_uratio_impl(char *buffer, size_t buffer_size,
                        opng_ulongest_impl_t num, opng_ulongest_impl_t denom,
                        int always_percent)
{
    /* (1) num/denom = 0/0                  ==> print "??%"
     * (2) num/denom = INFINITY             ==> print "INFTY%"
     * (3) 0 <= num/denom < 99.995%         ==> use the percent format "99.99%"
     *     if always_percent:
     * (4)    0.995 <= num/denom < INFINITY ==> use the percent format "999%"
     *     else:
     * (5)    0.995 <= num/denom < 99.995   ==> use the factor format "9.99x"
     * (6)    99.5 <= num/denom < INFINITY  ==> use the factor format "999x"
     *     end if
     */

    opng_ulongest_impl_t integer_part, remainder;
    unsigned int fractional_part, scale;
    double scaled_ratio;

    /* (1,2): num/denom = 0/0 or num/denom = INFINITY */
    if (denom == 0)
        return opng_snprintf_impl(buffer, buffer_size,
                                  num == 0 ? "??%%" : "INFTY%%");

    /* (3): 0 <= num/denom < 99.995% */
    /* num/denom < 99.995% <==> denom/(denom-num) < 20000 */
    if (num < denom && denom / (denom - num) < 20000)
    {
        scale = 10000;
        scaled_ratio = ((double)num * (double)scale) / (double)denom;
        fractional_part = (unsigned int)(scaled_ratio + 0.5);
        /* Adjust the scaled result in the event of a roundoff error. */
        /* Such error may occur only if the numerator is extremely large. */
        if (fractional_part >= scale)
            fractional_part = scale - 1;
        return opng_snprintf_impl(buffer, buffer_size,
                                  "%u.%02u%%",
                                  fractional_part / 100,
                                  fractional_part % 100);
    }

    /* Extract the integer part out of the fraction for the remaining cases. */
    integer_part = num / denom;
    remainder = num % denom;
    scale = 100;
    scaled_ratio = ((double)remainder * (double)scale) / (double)denom;
    fractional_part = (unsigned int)(scaled_ratio + 0.5);
    if (fractional_part >= scale)
    {
        fractional_part = 0;
        ++integer_part;
    }

    /* (4): 0.995 <= num/denom < INFINITY */
    if (always_percent)
        return opng_snprintf_impl(buffer, buffer_size,
                                  "%" OPNG_LONGEST_IMPL_FORMAT "u%02u%%",
                                  integer_part, fractional_part);

    /* (5): 0.995 <= num/denom < 99.995 */
    if (integer_part < 100)
        return opng_snprintf_impl(buffer, buffer_size,
                                  "%" OPNG_LONGEST_IMPL_FORMAT "u.%02ux",
                                  integer_part, fractional_part);

    /* (6): 99.5 <= num/denom < INFINITY */
    /* Round to the nearest integer. */
    /* Recalculate the integer part, for corner cases like 123.999. */
    integer_part = num / denom;
    if (remainder > (denom - 1) / 2)
        ++integer_part;
    return opng_snprintf_impl(buffer, buffer_size,
                              "%" OPNG_LONGEST_IMPL_FORMAT "ux",
                              integer_part);
}

/*
 * Converts a rational value to a compact factor string representation.
 */
int
opng_ulratio_to_factor_string(char *buffer, size_t buffer_size,
                              const struct opng_ulratio *ratio)
{
    opng_ulongest_impl_t num = ratio->num;
    opng_ulongest_impl_t denom = ratio->denom;
    return opng_sprint_uratio_impl(buffer, buffer_size, num, denom, 0);
}

/*
 * Converts a rational value to a compact percent string representation.
 */
int
opng_ulratio_to_percent_string(char *buffer, size_t buffer_size,
                               const struct opng_ulratio *ratio)
{
    opng_ulongest_impl_t num = ratio->num;
    opng_ulongest_impl_t denom = ratio->denom;
    return opng_sprint_uratio_impl(buffer, buffer_size, num, denom, 1);
}

#ifdef OPNG_LLONG_T_DEFINED

/*
 * Converts a rational value to a compact factor string representation.
 */
int
opng_ullratio_to_factor_string(char *buffer, size_t buffer_size,
                               const struct opng_ullratio *ratio)
{
    opng_ulongest_impl_t num = ratio->num;
    opng_ulongest_impl_t denom = ratio->denom;
    return opng_sprint_uratio_impl(buffer, buffer_size, num, denom, 0);
}

/*
 * Converts a rational value to a compact percent string representation.
 */
int
opng_ullratio_to_percent_string(char *buffer, size_t buffer_size,
                                const struct opng_ullratio *ratio)
{
    opng_ulongest_impl_t num = ratio->num;
    opng_ulongest_impl_t denom = ratio->denom;
    return opng_sprint_uratio_impl(buffer, buffer_size, num, denom, 1);
}

#endif  /* OPNG_LLONG_T_DEFINED */
