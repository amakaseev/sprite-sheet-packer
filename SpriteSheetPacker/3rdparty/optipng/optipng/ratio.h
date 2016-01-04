/*
 * ratio.h
 * Exact rational numbers.
 *
 * Copyright (C) 2003-2014 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef RATIO_H
#define RATIO_H

#include <stddef.h>


/* The following definitions exist for the benefit of pre-C99 and pre-C++11
 * compilers and runtimes that are unable to grok the long long type.
 */
#ifndef OPNG_LLONG_T_DEFINED

#include <limits.h>

#if defined LLONG_MAX && defined ULLONG_MAX
#if (LLONG_MAX >= LONG_MAX) && (ULLONG_MAX >= ULONG_MAX)
typedef long long opng_llong_t;
typedef unsigned long long opng_ullong_t;
#define OPNG_LLONG_MIN LLONG_MIN
#define OPNG_LLONG_MAX LLONG_MAX
#define OPNG_ULLONG_MAX ULLONG_MAX
#define OPNG_LLONG_C(value) value##LL
#define OPNG_ULLONG_C(value) value##ULL
#define OPNG_LLONG_T_DEFINED 1
#endif
#elif defined _I64_MAX && defined _UI64_MAX
#if defined _WIN32 || defined __WIN32__
typedef __int64 opng_llong_t;
typedef unsigned __int64 opng_ullong_t;
#define OPNG_LLONG_MIN _I64_MIN
#define OPNG_LLONG_MAX _I64_MAX
#define OPNG_ULLONG_MAX _UI64_MAX
#define OPNG_LLONG_C(value) value##i64
#define OPNG_ULLONG_C(value) value##ui64
#define OPNG_LLONG_T_DEFINED 1
#endif
#endif

#ifdef OPNG_LLONG_T_DEFINED
#if defined _WIN32 || defined __WIN32__
/* The "ll" format modifier may not work on Windows XP and earlier. */
#define OPNG_LLONG_FORMAT "I64"
#else
#define OPNG_LLONG_FORMAT "ll"
#endif
#endif

#endif  /* OPNG_LLONG_T_DEFINED */


#ifdef __cplusplus
extern "C" {
#endif


/*
 * The long rational type.
 */
struct opng_lratio
{
    long num;
    long denom;
};

/*
 * The unsigned long rational type.
 */
struct opng_ulratio
{
    unsigned long num;
    unsigned long denom;
};

#ifdef OPNG_LLONG_T_DEFINED

/*
 * The long long rational type.
 */
struct opng_llratio
{
    opng_llong_t num;
    opng_llong_t denom;
};

/*
 * The unsigned long long rational type.
 */
struct opng_ullratio
{
    opng_ullong_t num;
    opng_ullong_t denom;
};

#endif  /* OPNG_LLONG_T_DEFINED */


/*
 * Converts a rational value to a compact factor string representation.
 * Examples: 34/55 -> "61.82%", 55/34 -> "1.62x".
 *
 * The factor string has the following format:
 *
 *   "DD.DD%" if ratio < 99.995%
 *   "DD.DDx" if ratio >= 99.995% and ratio < 99.995
 *   "DDDx"   if ratio >= 99.995
 *   "??%"    if ratio == 0/0
 *   "INFTY%" if ratio >= 1/0
 *
 * The buffer shall contain the resulting string, or a part of it if the
 * buffer size is too small, always null-terminated.
 * The function shall return the number of characters stored, not including
 * the null-character terminator, or -1 if the buffer size is too small.
 */
int
opng_ulratio_to_factor_string(char *buffer, size_t buffer_size,
                              const struct opng_ulratio *ratio);

/*
 * Converts a rational value to a compact percent string representation.
 * Examples: 34/55 -> "61.82%", 55/34 -> "162%".
 *
 * This is the format "DD.DD%" for ratios below 99.995%, and the format
 * "DDD%" for ratios equal to or above 99.995%.
 *
 * The buffer shall contain the resulting string, or a part of it if the
 * buffer size is too small, always null-terminated.
 * The function shall return the number of characters stored, not including
 * the null-character terminator, or -1 if the buffer size is too small.
 */
int
opng_ulratio_to_percent_string(char *buffer, size_t buffer_size,
                               const struct opng_ulratio *ratio);

#ifdef OPNG_LLONG_T_DEFINED

/*
 * Converts a rational value to a compact factor string representation.
 * See opng_ulratio_to_factor_string.
 */
int
opng_ullratio_to_factor_string(char *buffer, size_t buffer_size,
                               const struct opng_ullratio *ratio);

/*
 * Converts a rational value to a compact percent string representation.
 * See opng_ulratio_to_percent_string.
 */
int
opng_ullratio_to_percent_string(char *buffer, size_t buffer_size,
                                const struct opng_ullratio *ratio);

#endif  /* OPNG_LLONG_T_DEFINED */

/*
 * TODO:
 * opng_[l,ll]ratio_to_factor_string
 * opng_[l,ul,ll,ull]ratio_to_factor_wstring
 * opng_[l,ll]ratio_to_percent_string
 * opng_[l,ul,ll,ull]ratio_to_percent_wstring
 */


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* RATIO_H */
