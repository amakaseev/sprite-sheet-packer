/*
 * strconv.h
 * String conversion utilities.
 *
 * Copyright (C) 2010-2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef STRCONV_H
#define STRCONV_H

#include <stdlib.h>

#include "bits.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Converts a numeric string to an unsigned long integer.
 * The input string represents an absolute integer number, with no +/- sign.
 * The output unsigned long value shall contain the converted number.
 * The function returns 0 on success, or -1 on failure.
 */
int
numeric_string_to_ulong(unsigned long *out_val,
                        const char *str, int allow_multiplier);

/*
 * Converts a rangeset string to a bitset.
 * The input string represents a rangeset of absolute integer numbers,
 * with no +/- signs.
 * The output bitset value shall contain the converted rangeset,
 * represented as an array of bits.
 * The function returns 0 on success, or -1 on failure.
 */
int
rangeset_string_to_bits(optk_bits_t *out_bits, const char *rangeset_str);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* STRCONV_H */
