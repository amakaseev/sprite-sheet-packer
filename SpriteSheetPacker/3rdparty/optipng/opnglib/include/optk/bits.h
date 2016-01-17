/*
 * optk/bits.h
 * Plain old bitset data type.
 *
 * Copyright (C) 2001-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef OPTK_BITS_H_
#define OPTK_BITS_H_

#include <limits.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 * The bitset type.
 */
typedef unsigned int optk_bits_t;


/*
 * The size operator (not restricted to optk_bits_t).
 */
#define OPTK_BITSIZEOF(object) (sizeof(object) * CHAR_BIT)


/*
 * Bitset limits.
 */
enum
{
    OPTK_BITS_ELT_MIN = 0,
    OPTK_BITS_ELT_MAX = (int)(OPTK_BITSIZEOF(optk_bits_t) - 1)
};


/*
 * Direct bitset access methods.
 */
#ifdef __cplusplus

inline int
optk_bits_test(optk_bits_t set, int elt)
{
    return (set & (1U << elt)) != 0;
}

inline void
optk_bits_set(optk_bits_t *set, int elt)
{
    *set |= (1U << elt);
}

inline void
optk_bits_reset(optk_bits_t *set, int elt)
{
    *set &= ~(1U << elt);
}

inline void
optk_bits_flip(optk_bits_t *set, int elt)
{
    *set ^= (1U << elt);
}

inline optk_bits_t
optk_bits__range__(int start_elt, int stop_elt)
{
    return ((1U << (stop_elt - start_elt) << 1) - 1) << start_elt;
}

inline int
optk_bits_test_all_in_range(optk_bits_t set, int start_elt, int stop_elt)
{
    return (start_elt <= stop_elt) ?
        ((~set & optk_bits__range__(start_elt, stop_elt)) == 0) : 1;
}

inline int
optk_bits_test_any_in_range(optk_bits_t set, int start_elt, int stop_elt)
{
    return (start_elt <= stop_elt) ?
        ((set & optk_bits__range__(start_elt, stop_elt)) != 0) : 0;
}

inline void
optk_bits_set_range(optk_bits_t *set, int start_elt, int stop_elt)
{
    if (start_elt <= stop_elt)
        *set |= (((1U << (stop_elt - start_elt) << 1) - 1) << start_elt);
}

inline void
optk_bits_reset_range(optk_bits_t *set, int start_elt, int stop_elt)
{
    if (start_elt <= stop_elt)
        *set &= ~(((1U << (stop_elt - start_elt) << 1) - 1) << start_elt);
}

inline void
optk_bits_flip_range(optk_bits_t *set, int start_elt, int stop_elt)
{
    if (start_elt <= stop_elt)
        *set ^= (((1U << (stop_elt - start_elt) << 1) - 1) << start_elt);
}

#else  /* !__cplusplus */

#define optk_bits_test(set, elt) \
    (((set) & (1U << (elt))) != 0)

#define optk_bits_set(set, elt) \
    (*(set) |= (1U << (elt)))

#define optk_bits_reset(set, elt) \
    (*(set) &= ~(1U << (elt)))

#define optk_bits_flip(set, elt) \
    (*(set) ^= (1U << (elt)))

#define optk_bits__range__(start_elt, stop_elt) \
    (((1U << ((stop_elt) - (start_elt)) << 1) - 1) << (start_elt))

#define optk_bits_test_all_in_range(set, start_elt, stop_elt) \
    (((start_elt) <= (stop_elt)) \
        ? (~(set) & optk_bits__range__(start_elt, stop_elt)) == 0 \
        : 1)

#define optk_bits_test_any_in_range(set, start_elt, stop_elt) \
    (((start_elt) <= (stop_elt)) \
        ? ((set) & optk_bits__range__(start_elt, stop_elt)) != 0 \
        : 0)

#define optk_bits_set_range(set, start_elt, stop_elt) \
    (*(set) |= ((start_elt) <= (stop_elt)) \
        ? optk_bits__range__(start_elt, stop_elt) \
        : 0U)

#define optk_bits_reset_range(set, start_elt, stop_elt) \
    (*(set) &= ((start_elt) <= (stop_elt)) \
        ? ~optk_bits__range__(start_elt, stop_elt) \
        : ~0U)

#define optk_bits_flip_range(set, start_elt, stop_elt) \
    (*(set) ^= ((start_elt) <= (stop_elt)) \
        ? optk_bits__range__(start_elt, stop_elt) \
        : 0U)

#endif  /* __cplusplus */


/*
 * Counts the number of elements in a bitset.
 *
 * The function returns the number of bits set to 1.
 */
unsigned int
optk_bits_count(optk_bits_t set);

/*
 * Finds the first element in a bitset.
 *
 * The function returns the position of the first bit set to 1,
 * or -1 if all bits are set to 0.
 */
int
optk_bits_find_first(optk_bits_t set);

/*
 * Finds the next element in a bitset.
 *
 * The function returns the position of the next bit set to 1,
 * or -1 if all the following bits are set to 0.
 */
int
optk_bits_find_next(optk_bits_t set, int elt);

/*
 * Finds the last element in a bitset.
 *
 * The function returns the position of the last bit set to 1,
 * or -1 if all bits are set to 0.
 */
int
optk_bits_find_last(optk_bits_t set);

/*
 * Finds the previous element in a bitset.
 *
 * The function returns the position of the previous bit set to 1,
 * or -1 if all the preceding bits are set to 0.
 */
int
optk_bits_find_prev(optk_bits_t set, int elt);

/*
 * Converts a rangeset string to a bitset.
 *
 * A rangeset string is an arbitrary sequence of elements ("N") and
 * ranges ("M-N" or "M-"), separated by ',' or ';'. Whitespace is
 * allowed around lexical elements, and is ignored.
 *
 * Here are a few examples, assuming OPTK_BITSIZEOF(optk_bits_t) == 16:
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
 * larger than OPTK_BITS_ELT_MAX), the function sets errno to ERANGE.
 * If the input is invalid, the function sets errno to EINVAL and
 * returns 0 (i.e. the empty set).
 */
optk_bits_t
optk_rangeset_string_to_bits(const char *str, size_t *end_idx);

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
optk_bits_to_rangeset_string(char *sbuf, size_t sbuf_size, optk_bits_t set);

/*
 * TODO:
 * optk_rangeset_wstring_to_bits
 * optk_bits_to_rangeset_wstring
 */


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPTK_BITS_H_ */
