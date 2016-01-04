/*
 * bitset.h
 * Plain old bitset data type.
 *
 * Copyright (C) 2001-2014 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef BITSET_H
#define BITSET_H

#include <limits.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 * The bitset type.
 */
typedef unsigned int opng_bitset_t;


/*
 * The size operator (not restricted to opng_bitset_t).
 */
#define OPNG_BITSIZEOF(object) (sizeof(object) * CHAR_BIT)


/*
 * Bitset limits.
 */
enum
{
    OPNG_BITSET_ELT_MIN = 0,
    OPNG_BITSET_ELT_MAX = (int)(OPNG_BITSIZEOF(opng_bitset_t) - 1)
};


/*
 * Direct bitset access methods.
 */
#ifdef __cplusplus

inline int
opng_bitset_test(opng_bitset_t set, int elt)
{
    return (set & (1U << elt)) != 0;
}

inline void
opng_bitset_set(opng_bitset_t *set, int elt)
{
    *set |= (1U << elt);
}

inline void
opng_bitset_reset(opng_bitset_t *set, int elt)
{
    *set &= ~(1U << elt);
}

inline void
opng_bitset_flip(opng_bitset_t *set, int elt)
{
    *set ^= (1U << elt);
}

inline opng_bitset_t
opng_bitset__range__(int start_elt, int stop_elt)
{
    return ((1U << (stop_elt - start_elt) << 1) - 1) << start_elt;
}

inline int
opng_bitset_test_all_in_range(opng_bitset_t set, int start_elt, int stop_elt)
{
    return (start_elt <= stop_elt) ?
        ((~set & opng_bitset__range__(start_elt, stop_elt)) == 0) : 1;
}

inline int
opng_bitset_test_any_in_range(opng_bitset_t set, int start_elt, int stop_elt)
{
    return (start_elt <= stop_elt) ?
        ((set & opng_bitset__range__(start_elt, stop_elt)) != 0) : 0;
}

inline void
opng_bitset_set_range(opng_bitset_t *set, int start_elt, int stop_elt)
{
    if (start_elt <= stop_elt)
        *set |= (((1U << (stop_elt - start_elt) << 1) - 1) << start_elt);
}

inline void
opng_bitset_reset_range(opng_bitset_t *set, int start_elt, int stop_elt)
{
    if (start_elt <= stop_elt)
        *set &= ~(((1U << (stop_elt - start_elt) << 1) - 1) << start_elt);
}

inline void
opng_bitset_flip_range(opng_bitset_t *set, int start_elt, int stop_elt)
{
    if (start_elt <= stop_elt)
        *set ^= (((1U << (stop_elt - start_elt) << 1) - 1) << start_elt);
}

#else  /* !__cplusplus */

#define opng_bitset_test(set, elt) \
    (((set) & (1U << (elt))) != 0)

#define opng_bitset_set(set, elt) \
    (*(set) |= (1U << (elt)))

#define opng_bitset_reset(set, elt) \
    (*(set) &= ~(1U << (elt)))

#define opng_bitset_flip(set, elt) \
    (*(set) ^= (1U << (elt)))

#define opng_bitset__range__(start_elt, stop_elt) \
    (((1U << ((stop_elt) - (start_elt)) << 1) - 1) << (start_elt))

#define opng_bitset_test_all_in_range(set, start_elt, stop_elt) \
    (((start_elt) <= (stop_elt)) \
        ? (~(set) & opng_bitset__range__(start_elt, stop_elt)) == 0 \
        : 1)

#define opng_bitset_test_any_in_range(set, start_elt, stop_elt) \
    (((start_elt) <= (stop_elt)) \
        ? ((set) & opng_bitset__range__(start_elt, stop_elt)) != 0 \
        : 0)

#define opng_bitset_set_range(set, start_elt, stop_elt) \
    (*(set) |= ((start_elt) <= (stop_elt)) \
        ? opng_bitset__range__(start_elt, stop_elt) \
        : 0U)

#define opng_bitset_reset_range(set, start_elt, stop_elt) \
    (*(set) &= ((start_elt) <= (stop_elt)) \
        ? ~opng_bitset__range__(start_elt, stop_elt) \
        : ~0U)

#define opng_bitset_flip_range(set, start_elt, stop_elt) \
    (*(set) ^= ((start_elt) <= (stop_elt)) \
        ? opng_bitset__range__(start_elt, stop_elt) \
        : 0U)

#endif  /* __cplusplus */


/*
 * Counts the number of elements in a bitset.
 *
 * The function returns the number of bits set to 1.
 */
unsigned int
opng_bitset_count(opng_bitset_t set);

/*
 * Finds the first element in a bitset.
 *
 * The function returns the position of the first bit set to 1,
 * or -1 if all bits are set to 0.
 */
int
opng_bitset_find_first(opng_bitset_t set);

/*
 * Finds the next element in a bitset.
 *
 * The function returns the position of the next bit set to 1,
 * or -1 if all the following bits are set to 0.
 */
int
opng_bitset_find_next(opng_bitset_t set, int elt);

/*
 * Finds the last element in a bitset.
 *
 * The function returns the position of the last bit set to 1,
 * or -1 if all bits are set to 0.
 */
int
opng_bitset_find_last(opng_bitset_t set);

/*
 * Finds the previous element in a bitset.
 *
 * The function returns the position of the previous bit set to 1,
 * or -1 if all the preceding bits are set to 0.
 */
int
opng_bitset_find_prev(opng_bitset_t set, int elt);

/*
 * Converts a rangeset string to a bitset.
 *
 * A rangeset string is an arbitrary sequence of elements ("N") and
 * ranges ("M-N" or "M-"), separated by ',' or ';'. Whitespace is
 * allowed around lexical elements, and is ignored.
 *
 * Here are a few examples, assuming OPNG_BITSIZEOF(opng_bitset_t) == 16:
 *  "0,3,5-7"  => 0000000011101001
 *  "0-3,5,7-" => 1111111110101111
 *  "8-,4"     => 1111111100010000
 *  ""         => 0000000000000000
 *  "8-4"      => 0000000000000000, errno = ERANGE
 *  "99"       => 1000000000000000, errno = ERANGE
 *  "invalid"  => 0000000000000000, errno = EINVAL
 *
 * If end_idx is not null, the function sets *end_idx to point to
 * the character that stopped the scan. If the input is invalid and
 * end_idx is not null, the function sets *end_idx to 0.
 *
 * The function returns the value of the converted bitset. If the
 * input contains non-representable elements or ranges (e.g. elements
 * larger than OPNG_BITSET_ELT_MAX), the function sets errno to ERANGE.
 * If the input is invalid, the function sets errno to EINVAL and
 * returns 0 (i.e. the empty set).
 */
opng_bitset_t
opng_rangeset_string_to_bitset(const char *str, size_t *end_idx);

/*
 * Converts a bitset to a rangeset string.
 *
 * The function converts the bitset to a rangeset string representation
 * and attempts to store it in sbuf, if sbuf is large enough. Otherwise,
 * it leaves sbuf intact.
 *
 * The function returns the length of the rangeset string representation.
 */
size_t
opng_bitset_to_rangeset_string(char *sbuf, size_t sbuf_size, opng_bitset_t set);

/*
 * TODO:
 * opng_rangeset_wstring_to_bitset
 * opng_bitset_to_rangeset_wstring
 */


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* BITSET_H */
