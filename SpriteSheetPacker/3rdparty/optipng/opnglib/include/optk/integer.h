/*
 * optk/integer.h
 * Integer types and associated macros.
 *
 * Copyright (C) 2011-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef OPTK_INTEGER_H_
#define OPTK_INTEGER_H_


/*
 * The macros OPTK__name__ are for private use only.
 * Applications should pretend not to know about them.
 */

/*
 * Auto-configure OPTK__STDC99__.
 */
#if defined __STDC_VERSION__
# if __STDC_VERSION__ >= 199901L
#  ifndef OPTK__STDC99__
#   define OPTK__STDC99__ 1
#  endif
# endif
#else
# include <limits.h>
# if defined SSIZE_MAX && defined _POSIX_SSIZE_MAX
#  if SSIZE_MAX > 0 && _POSIX_SSIZE_MAX > 0
#   include <unistd.h>
#   if defined _POSIX_VERSION
#    if _POSIX_VERSION >= 200112L
#     ifndef OPTK__STDC99__
#      define OPTK__STDC99__ 1
#     endif
#    endif
#   endif
#  endif
# endif
#endif

#if defined OPTK__STDC99__

#ifndef __STDC_LIMIT_MACROS
# define __STDC_LIMIT_MACROS 1
#endif
#ifndef __STDC_CONSTANT_MACROS
# define __STDC_CONSTANT_MACROS 1
#endif
#ifndef __STDC_FORMAT_MACROS
# define __STDC_FORMAT_MACROS 1
#endif
#include <stdint.h>
#include <inttypes.h>

/*
 * Use the Standard C minimum-width integer types.
 *
 * The exact-width types intN_t and uintN_t are not guaranteed to exist
 * for all N=8,16,32,64. For example, certain dedicated CPUs may handle
 * 32-bit and 64-bit integers only, with sizeof(int)==sizeof(char)==1.
 * On the other hand, any minimum-width integer type is guaranteed to be
 * the same as its exact-width counterpart, when the latter does exist.
 *
 * Since exact-width integer semantics is not strictly required, we use
 * int_leastN_t and uint_leastN_t, which are required to exist for all
 * N=8,16,32,64.
 */

typedef int_least8_t   optk_int8_t;
typedef uint_least8_t  optk_uint8_t;
typedef int_least16_t  optk_int16_t;
typedef uint_least16_t optk_uint16_t;
typedef int_least32_t  optk_int32_t;
typedef uint_least32_t optk_uint32_t;
typedef int_least64_t  optk_int64_t;
typedef uint_least64_t optk_uint64_t;

#define OPTK_INT8_MIN   INT_LEAST8_MIN
#define OPTK_INT8_MAX   INT_LEAST8_MAX
#define OPTK_UINT8_MAX  UINT_LEAST8_MAX
#define OPTK_INT16_MIN  INT_LEAST16_MIN
#define OPTK_INT16_MAX  INT_LEAST16_MAX
#define OPTK_UINT16_MAX UINT_LEAST16_MAX
#define OPTK_INT32_MIN  INT_LEAST32_MIN
#define OPTK_INT32_MAX  INT_LEAST32_MAX
#define OPTK_UINT32_MAX UINT_LEAST32_MAX
#define OPTK_INT64_MIN  INT_LEAST64_MIN
#define OPTK_INT64_MAX  INT_LEAST64_MAX
#define OPTK_UINT64_MAX UINT_LEAST64_MAX

#define OPTK_INT8_C(value)   INT8_C(value)
#define OPTK_UINT8_C(value)  UINT8_C(value)
#define OPTK_INT16_C(value)  INT16_C(value)
#define OPTK_UINT16_C(value) UINT16_C(value)
#define OPTK_INT32_C(value)  INT32_C(value)
#define OPTK_UINT32_C(value) UINT32_C(value)
#define OPTK_INT64_C(value)  INT64_C(value)
#define OPTK_UINT64_C(value) UINT64_C(value)

#define OPTK_SCNd8  SCNdLEAST8
#define OPTK_SCNi8  SCNiLEAST8
#define OPTK_SCNu8  SCNuLEAST8
#define OPTK_SCNo8  SCNoLEAST8
#define OPTK_SCNx8  SCNxLEAST8
#define OPTK_SCNd16 SCNdLEAST16
#define OPTK_SCNi16 SCNiLEAST16
#define OPTK_SCNu16 SCNuLEAST16
#define OPTK_SCNo16 SCNoLEAST16
#define OPTK_SCNx16 SCNxLEAST16
#define OPTK_SCNd32 SCNdLEAST32
#define OPTK_SCNi32 SCNiLEAST32
#define OPTK_SCNu32 SCNuLEAST32
#define OPTK_SCNo32 SCNoLEAST32
#define OPTK_SCNx32 SCNxLEAST32
#define OPTK_SCNd64 SCNdLEAST64
#define OPTK_SCNi64 SCNiLEAST64
#define OPTK_SCNu64 SCNuLEAST64
#define OPTK_SCNo64 SCNoLEAST64
#define OPTK_SCNx64 SCNxLEAST64

#define OPTK_PRId8  PRIdLEAST8
#define OPTK_PRIi8  PRIiLEAST8
#define OPTK_PRIu8  PRIuLEAST8
#define OPTK_PRIo8  PRIoLEAST8
#define OPTK_PRIx8  PRIxLEAST8
#define OPTK_PRIX8  PRIXLEAST8
#define OPTK_PRId16 PRIdLEAST16
#define OPTK_PRIi16 PRIiLEAST16
#define OPTK_PRIu16 PRIuLEAST16
#define OPTK_PRIo16 PRIoLEAST16
#define OPTK_PRIx16 PRIxLEAST16
#define OPTK_PRIX16 PRIXLEAST16
#define OPTK_PRId32 PRIdLEAST32
#define OPTK_PRIi32 PRIiLEAST32
#define OPTK_PRIu32 PRIuLEAST32
#define OPTK_PRIo32 PRIoLEAST32
#define OPTK_PRIx32 PRIxLEAST32
#define OPTK_PRIX32 PRIXLEAST32
#define OPTK_PRId64 PRIdLEAST64
#define OPTK_PRIi64 PRIiLEAST64
#define OPTK_PRIu64 PRIuLEAST64
#define OPTK_PRIo64 PRIoLEAST64
#define OPTK_PRIx64 PRIxLEAST64
#define OPTK_PRIX64 PRIXLEAST64

#else  /* !OPTK__STDC99__ */

/*
 * Auto-configure OPTK__LLONG_TYPE__ and its associated macros.
 */
#ifndef OPTK__LLONG_TYPE__
# if defined _I64_MIN && defined _I64_MAX && defined _UI64_MAX
#  if (defined _WIN32 || defined __WIN32__) && _I64_MAX > LONG_MAX
#   define OPTK__LLONG_TYPE__ __int64
#   define OPTK__LLONG_MIN__  _I64_MIN
#   define OPTK__LLONG_MAX__  _I64_MAX
#   define OPTK__ULLONG_MAX__ _UI64_MAX
#   if defined LLONG_MIN && defined LLONG_MAX && defined ULLONG_MAX
#    if !(_I64_MAX == LLONG_MAX && _UI64_MAX == ULLONG_MAX)
#     error Preprocessor assertion failed: long long limits mismatch
#    endif
#    define OPTK__LLONG_C__(value)  value##LL
#    define OPTK__ULLONG_C__(value) value##ULL
#   else
#    define OPTK__LLONG_C__(value)  value##i64
#    define OPTK__ULLONG_C__(value) value##ui64
#   endif
#   define OPTK__LLONG_SCNd__  "I64d"
#   define OPTK__LLONG_SCNi__  "I64i"
#   define OPTK__ULLONG_SCNu__ "I64u"
#   define OPTK__ULLONG_SCNo__ "I64o"
#   define OPTK__ULLONG_SCNx__ "I64x"
#   define OPTK__LLONG_PRId__  "I64d"
#   define OPTK__LLONG_PRIi__  "I64i"
#   define OPTK__ULLONG_PRIu__ "I64u"
#   define OPTK__ULLONG_PRIo__ "I64o"
#   define OPTK__ULLONG_PRIx__ "I64x"
#   define OPTK__ULLONG_PRIX__ "I64X"
#  endif
# endif
#endif
#ifndef OPTK__LLONG_TYPE__
# if defined LLONG_MIN && defined LLONG_MAX && defined ULLONG_MAX
#  if !(LLONG_MAX >= 0x7fffffffffffffffLL && LLONG_MIN <= -LLONG_MAX)
#   error Preprocessor assertion failed: LLONG_MIN and LLONG_MAX are incorrect
#  endif
#  if !(ULLONG_MAX >= 0xffffffffffffffffLL)
#   error Preprocessor assertion failed: ULLONG_MAX is incorrect
#  endif
#  define OPTK__LLONG_TYPE__      long long
#  define OPTK__LLONG_MIN__       LLONG_MIN
#  define OPTK__LLONG_MAX__       LLONG_MAX
#  define OPTK__ULLONG_MAX__      ULLONG_MAX
#  define OPTK__LLONG_C__(value)  value##LL
#  define OPTK__ULLONG_C__(value) value##ULL
#  define OPTK__LLONG_SCNd__      "lld"
#  define OPTK__LLONG_SCNi__      "lli"
#  define OPTK__ULLONG_SCNu__     "llu"
#  define OPTK__ULLONG_SCNo__     "llo"
#  define OPTK__ULLONG_SCNx__     "llx"
#  define OPTK__LLONG_PRId__      "lld"
#  define OPTK__LLONG_PRIi__      "lli"
#  define OPTK__ULLONG_PRIu__     "llu"
#  define OPTK__ULLONG_PRIo__     "llo"
#  define OPTK__ULLONG_PRIx__     "llx"
#  define OPTK__ULLONG_PRIX__     "llX"
# endif
#endif

/*
 * Auto-configure OPTK__I8_I16_I32_I64__.
 */
#if SCHAR_MAX == 0x7f && SHRT_MAX == 0x7fff && INT_MAX == 0x7fffffffL
# if LONG_MAX > INT_MAX
#  if LONG_MAX == 0x7fffffffffffffffL
#   define OPTK__I8_I16_I32_I64__ 1
#  endif
# elif defined OPTK__LLONG_MAX__
#  if OPTK__LLONG_MAX__ == OPTK__LLONG_C__(0x7fffffffffffffff)
#   define OPTK__I8_I16_I32_I64__ 1
#  endif
# endif
#endif

#if defined OPTK__I8_I16_I32_I64__

/*
 * Use the common C integer types under the ILP32 or LP64 data models.
 */

typedef signed char                 optk_int8_t;
typedef unsigned char               optk_uint8_t;
typedef short                       optk_int16_t;
typedef unsigned short              optk_uint16_t;
typedef int                         optk_int32_t;
typedef unsigned int                optk_uint32_t;
#if LONG_MAX > INT_MAX
typedef long                        optk_int64_t;
typedef unsigned long               optk_uint64_t;
#else
typedef OPTK__LLONG_TYPE__          optk_int64_t;
typedef unsigned OPTK__LLONG_TYPE__ optk_uint64_t;
#endif

#define OPTK_INT8_MIN    SCHAR_MIN
#define OPTK_INT8_MAX    SCHAR_MAX
#define OPTK_UINT8_MAX   UCHAR_MAX
#define OPTK_INT16_MIN   SHRT_MIN
#define OPTK_INT16_MAX   SHRT_MAX
#define OPTK_UINT16_MAX  USHRT_MAX
#define OPTK_INT32_MIN   INT_MIN
#define OPTK_INT32_MAX   INT_MAX
#define OPTK_UINT32_MAX  UINT_MAX
#if LONG_MAX > INT_MAX
# define OPTK_INT64_MIN  LONG_MIN
# define OPTK_INT64_MAX  LONG_MAX
# define OPTK_UINT64_MAX ULONG_MAX
#else
# define OPTK_INT64_MIN  OPTK__LLONG_MIN__
# define OPTK_INT64_MAX  OPTK__LLONG_MAX__
# define OPTK_UINT64_MAX OPTK__ULLONG_MAX__
#endif

#define OPTK_INT8_C(value)    (SCHAR_MAX - SCHAR_MAX + (value))
#define OPTK_UINT8_C(value)   (UCHAR_MAX - UCHAR_MAX + (value))
#define OPTK_INT16_C(value)   (SHRT_MAX - SHRT_MAX + (value))
#define OPTK_UINT16_C(value)  (USHRT_MAX - USHRT_MAX + (value))
#define OPTK_INT32_C(value)   (INT_MAX - INT_MAX + (value))
#define OPTK_UINT32_C(value)  (UINT_MAX - UINT_MAX + (value))
#if LONG_MAX > INT_MAX
# define OPTK_INT64_C(value)  value##L
# define OPTK_UINT64_C(value) value##UL
#else
# define OPTK_INT64_C(value)  OPTK__LLONG_C__(value)
# define OPTK_UINT64_C(value) OPTK__ULLONG_C__(value)
#endif

#if 0  /* not available in pre-C99 */
# define OPTK_SCNd8  "hhd"
# define OPTK_SCNi8  "hhi"
# define OPTK_SCNu8  "hhu"
# define OPTK_SCNo8  "hho"
# define OPTK_SCNx8  "hhx"
#endif
#define OPTK_SCNd16  "hd"
#define OPTK_SCNi16  "hi"
#define OPTK_SCNu16  "hu"
#define OPTK_SCNo16  "ho"
#define OPTK_SCNx16  "hx"
#define OPTK_SCNd32  "d"
#define OPTK_SCNi32  "i"
#define OPTK_SCNu32  "u"
#define OPTK_SCNo32  "o"
#define OPTK_SCNx32  "x"
#if LONG_MAX > INT_MAX
# define OPTK_SCNd64 "ld"
# define OPTK_SCNi64 "li"
# define OPTK_SCNu64 "lu"
# define OPTK_SCNo64 "lo"
# define OPTK_SCNx64 "lx"
#else
# define OPTK_SCNd64 OPTK__LLONG_SCNd__
# define OPTK_SCNi64 OPTK__LLONG_SCNi__
# define OPTK_SCNu64 OPTK__ULLONG_SCNu__
# define OPTK_SCNo64 OPTK__ULLONG_SCNo__
# define OPTK_SCNx64 OPTK__ULLONG_SCNx__
#endif

#define OPTK_PRId8   "d"
#define OPTK_PRIi8   "i"
#define OPTK_PRIu8   "u"
#define OPTK_PRIo8   "o"
#define OPTK_PRIx8   "x"
#define OPTK_PRIX8   "X"
#define OPTK_PRId16  "d"
#define OPTK_PRIi16  "i"
#define OPTK_PRIu16  "u"
#define OPTK_PRIo16  "o"
#define OPTK_PRIx16  "x"
#define OPTK_PRIX16  "X"
#define OPTK_PRId32  "d"
#define OPTK_PRIi32  "i"
#define OPTK_PRIu32  "u"
#define OPTK_PRIo32  "o"
#define OPTK_PRIx32  "x"
#define OPTK_PRIX32  "X"
#if LONG_MAX > INT_MAX
# define OPTK_PRId64 "ld"
# define OPTK_PRIi64 "li"
# define OPTK_PRIu64 "lu"
# define OPTK_PRIo64 "lo"
# define OPTK_PRIx64 "lx"
# define OPTK_PRIX64 "lX"
#else
# define OPTK_PRId64 OPTK__LLONG_PRId__
# define OPTK_PRIi64 OPTK__LLONG_PRIi__
# define OPTK_PRIu64 OPTK__ULLONG_PRIu__
# define OPTK_PRIo64 OPTK__ULLONG_PRIo__
# define OPTK_PRIx64 OPTK__ULLONG_PRIx__
# define OPTK_PRIX64 OPTK__ULLONG_PRIX__
#endif

#else  /* !OPTK__I8_I16_I32_I64__ */

# error Could not auto-configure the integer types.

#endif  /* ?OPTK__I8_I16_I32_I64__ */

#endif  /* ?OPTK__STDC99__ */


#endif  /* OPTK_INTEGER_H_ */
