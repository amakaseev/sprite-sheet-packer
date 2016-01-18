/*
 * optk/bits.c
 * Plain old bitset data type.
 *
 * Copyright (C) 2001-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include "bits.h"

#include <ctype.h>
#include <errno.h>
#include <stddef.h>


/*
 * Private helper macros: _optk_MIN and _optk_MAX.
 */
#define _optk_MIN(a, b) \
    ((a) < (b) ? (a) : (b))
#define _optk_MAX(a, b) \
    ((a) > (b) ? (a) : (b))

/*
 * Private helper macro: _optk_PTR_SPAN_PRED.
 *
 * Spans the given pointer past the elements that satisfy the given predicate.
 * E.g., _optk_PTR_SPAN_PRED(str, isspace) moves str past the leading
 * whitespace.
 */
#define _optk_PTR_SPAN_PRED(ptr, predicate) \
    { while (predicate(*(ptr))) ++(ptr); }


/*
 * Counts the number of elements in a bitset.
 */
unsigned int
optk_bits_count(optk_bits_t set)
{
    unsigned int result;

    /* Apply Wegner's method. */
    result = 0;
    while (set != 0)
    {
        set &= (set - 1);
        ++result;
    }
    return result;
}

/*
 * Finds the first element in a bitset.
 */
int
optk_bits_find_first(optk_bits_t set)
{
    int i;

    for (i = 0; i <= OPTK_BITS_ELT_MAX; ++i)
    {
        if (optk_bits_test(set, i))
            return i;
    }
    return -1;
}

/*
 * Finds the next element in a bitset.
 */
int
optk_bits_find_next(optk_bits_t set, int elt)
{
    int i;

    for (i = _optk_MAX(elt, -1) + 1; i <= OPTK_BITS_ELT_MAX; ++i)
    {
        if (optk_bits_test(set, i))
            return i;
    }
    return -1;
}

/*
 * Finds the last element in a bitset.
 */
int
optk_bits_find_last(optk_bits_t set)
{
    int i;

    for (i = OPTK_BITS_ELT_MAX; i >= 0; --i)
    {
        if (optk_bits_test(set, i))
            return i;
    }
    return -1;
}

/*
 * Finds the previous element in a bitset.
 */
int
optk_bits_find_prev(optk_bits_t set, int elt)
{
    int i;

    for (i = _optk_MIN(elt, OPTK_BITS_ELT_MAX + 1) - 1; i >= 0; --i)
    {
        if (optk_bits_test(set, i))
            return i;
    }
    return -1;
}

/*
 * Converts a rangeset string to a bitset.
 */
optk_bits_t
optk_rangeset_string_to_bits(const char *str, size_t *end_idx)
{
    optk_bits_t result;
    const char *ptr;
    int state;
    int num, num1, num2;
    int out_of_range;

    result = 0;  /* empty */
    ptr = str;
    state = 0;
    out_of_range = 0;
    num1 = num2 = -1;

    for ( ; ; )
    {
        _optk_PTR_SPAN_PRED(ptr, isspace);
        switch (state)
        {
        case 0:  /* "" */
        case 2:  /* "N-" */
            /* Expecting number; go to next state. */
            if (*ptr >= '0' && *ptr <= '9')
            {
                num = 0;
                do
                {
                    num = 10 * num + (*ptr - '0');
                    if (num > OPTK_BITS_ELT_MAX)
                    {
                        out_of_range = 1;
                        num = OPTK_BITS_ELT_MAX;
                    }
                    ++ptr;
                } while (*ptr >= '0' && *ptr <= '9');
                if (state == 0)
                    num1 = num;
                num2 = num;
                ++state;
                continue;
            }
            break;
        case 1:  /* "N" */
            /* Expecting range operator; go to next state. */
            if (*ptr == '-')
            {
                ++ptr;
                num2 = OPTK_BITS_ELT_MAX;
                ++state;
                continue;
            }
            break;
        }

        if (state > 0)  /* "N", "N-" or "N-N" */
        {
            /* Store the partial result; go to state 0. */
            state = 0;
            if (num2 > OPTK_BITS_ELT_MAX)
            {
                out_of_range = 1;
                num2 = OPTK_BITS_ELT_MAX;
            }
            if (num1 <= num2)
                optk_bits_set_range(&result, num1, num2);
            else
                out_of_range = 1;
        }

        if (*ptr == ',' || *ptr == ';')
        {
            /* Separator: continue the loop. */
            ++ptr;
            continue;
        }
        else
        {
            /* Unexpected character or end of string: break the loop. */
            break;
        }
    }

    if (num1 == -1)
    {
        /* There were no partial results. */
        if (end_idx != NULL)
            *end_idx = 0;
        /* No EINVAL here: the empty set is a valid input. */
        return 0;
    }
    if (end_idx != NULL)
        *end_idx = (size_t)(ptr - str);
#ifdef ERANGE
    if (out_of_range)
        errno = ERANGE;
#endif
    return result;
}

/*
 * Converts a bitset to a rangeset string.
 */
size_t
optk_bits_to_rangeset_string(char *sbuf, size_t sbuf_size, optk_bits_t set);
/* TODO */
