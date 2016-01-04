/*
 * bitset.c
 * Plain old bitset data type.
 *
 * Copyright (C) 2001-2014 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include "bitset.h"

#include <ctype.h>
#include <errno.h>
#include <stddef.h>


/*
 * Private helper macros: opng__MIN__ and opng__MAX__.
 */
#define opng__MIN__(a, b) \
    ((a) < (b) ? (a) : (b))
#define opng__MAX__(a, b) \
    ((a) > (b) ? (a) : (b))

/*
 * Private helper macro: opng__PTR_SPAN_PRED__.
 *
 * Spans the given pointer past the elements that satisfy the given predicate.
 * E.g., opng__PTR_SPAN_PRED__(str, isspace) moves str past the leading
 * whitespace.
 */
#define opng__PTR_SPAN_PRED__(ptr, predicate) \
    { while (predicate(*(ptr))) ++(ptr); }


/*
 * Counts the number of elements in a bitset.
 */
unsigned int
opng_bitset_count(opng_bitset_t set)
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
opng_bitset_find_first(opng_bitset_t set)
{
    int i;

    for (i = 0; i <= OPNG_BITSET_ELT_MAX; ++i)
    {
        if (opng_bitset_test(set, i))
            return i;
    }
    return -1;
}

/*
 * Finds the next element in a bitset.
 */
int
opng_bitset_find_next(opng_bitset_t set, int elt)
{
    int i;

    for (i = opng__MAX__(elt, -1) + 1; i <= OPNG_BITSET_ELT_MAX; ++i)
    {
        if (opng_bitset_test(set, i))
            return i;
    }
    return -1;
}

/*
 * Finds the last element in a bitset.
 */
int
opng_bitset_find_last(opng_bitset_t set)
{
    int i;

    for (i = OPNG_BITSET_ELT_MAX; i >= 0; --i)
    {
        if (opng_bitset_test(set, i))
            return i;
    }
    return -1;
}

/*
 * Finds the previous element in a bitset.
 */
int
opng_bitset_find_prev(opng_bitset_t set, int elt)
{
    int i;

    for (i = opng__MIN__(elt, OPNG_BITSET_ELT_MAX + 1) - 1; i >= 0; --i)
    {
        if (opng_bitset_test(set, i))
            return i;
    }
    return -1;
}

/*
 * Converts a rangeset string to a bitset.
 */
opng_bitset_t
opng_rangeset_string_to_bitset(const char *str, size_t *end_idx)
{
    opng_bitset_t result;
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
        opng__PTR_SPAN_PRED__(ptr, isspace);
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
                    if (num > OPNG_BITSET_ELT_MAX)
                    {
                        out_of_range = 1;
                        num = OPNG_BITSET_ELT_MAX;
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
                num2 = OPNG_BITSET_ELT_MAX;
                ++state;
                continue;
            }
            break;
        }

        if (state > 0)  /* "N", "N-" or "N-N" */
        {
            /* Store the partial result; go to state 0. */
            state = 0;
            if (num2 > OPNG_BITSET_ELT_MAX)
            {
                out_of_range = 1;
                num2 = OPNG_BITSET_ELT_MAX;
            }
            if (num1 <= num2)
                opng_bitset_set_range(&result, num1, num2);
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
opng_bitset_to_rangeset_string(char *sbuf, size_t sbuf_size, opng_bitset_t set);
/* TODO */
