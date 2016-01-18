/*
 * strconv.c
 * String conversion utilities.
 *
 * Copyright (C) 2010-2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include "strconv.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>


#define PTR_SPAN_PRED(ptr, predicate)  \
    { while (predicate(*(ptr))) ++(ptr); }


/*
 * Converts a numeric string to an unsigned long integer.
 */
int
numeric_string_to_ulong(unsigned long *out_val,
                        const char *str, int allow_multiplier)
{
    const char *begin_ptr;
    char *end_ptr;
    unsigned long multiplier;

    /* Extract the value from the string. */
    /* Do not allow the minus sign, not even for -0. */
    begin_ptr = str;
    PTR_SPAN_PRED(begin_ptr, isspace);
    end_ptr = (char *)begin_ptr;
    if (*begin_ptr >= '0' && *begin_ptr <= '9')
        *out_val = strtoul(begin_ptr, &end_ptr, 10);
    if (begin_ptr == end_ptr)
    {
        errno = EINVAL;  /* matching failure */
        *out_val = 0;
        return -1;
    }

    if (allow_multiplier)
    {
        /* Check for the following SI suffixes:
         *   'K' or 'k': kibi (1024)
         *   'M':        mebi (1024 * 1024)
         *   'G':        gibi (1024 * 1024 * 1024)
         */
        if (*end_ptr == 'k' || *end_ptr == 'K')
        {
            ++end_ptr;
            multiplier = 1024UL;
        }
        else if (*end_ptr == 'M')
        {
            ++end_ptr;
            multiplier = 1024UL * 1024UL;
        }
        else if (*end_ptr == 'G')
        {
            ++end_ptr;
            multiplier = 1024UL * 1024UL * 1024UL;
        }
        else
            multiplier = 1;
        if (multiplier > 1)
        {
            if (*out_val > ULONG_MAX / multiplier)
            {
                errno = ERANGE;  /* overflow */
                *out_val = ULONG_MAX;
            }
            else
                *out_val *= multiplier;
        }
    }

    /* Check for trailing garbage. */
    PTR_SPAN_PRED(end_ptr, isspace);
    if (*end_ptr != 0)
    {
        errno = EINVAL;  /* garbage in input */
        return -1;
    }

    return 0;
}

/*
 * Converts a rangeset string to a bitset.
 */
int
rangeset_string_to_bits(optk_bits_t *out_bits, const char *rangeset_str)
{
    const char *ptr;
    size_t end_idx;

    *out_bits = optk_rangeset_string_to_bits(rangeset_str, &end_idx);

    /* Check for trailing garbage. */
    ptr = rangeset_str + end_idx;
    PTR_SPAN_PRED(ptr, isspace);
    if (*ptr != 0)
    {
        errno = EINVAL;  /* matching failure or trailing garbage in input */
        *out_bits = 0;
        return -1;
    }

    return 0;
}
