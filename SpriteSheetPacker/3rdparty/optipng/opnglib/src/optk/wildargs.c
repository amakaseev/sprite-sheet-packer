/*
 * optk/wildargs.c
 * Automatic command-line wildcard expansion for environments
 * that are not based on the Un*x shell.
 *
 * Copyright (C) 2003-2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

/*
 * The following code is inspired from MinGW32 by Colin Peters.
 */
#if defined _MSC_VER && (defined _WIN32 || defined _WIN64)
int _dowildcard = 1;
#endif

/*
 * The following code is inspired from BMP2PNG by MIYASAKA Masaru.
 */
#if defined __BORLANDC__ && (defined __WIN32__ || defined __WIN64__)
#include <wildargs.h>
typedef void _RTLENTRY (* _RTLENTRY _argv_expand_fn)(char *, _PFN_ADDARG);
typedef void _RTLENTRY (* _RTLENTRY _wargv_expand_fn)(wchar_t *, _PFN_ADDARG);
_argv_expand_fn _argv_expand_ptr = _expand_wild;
_wargv_expand_fn _wargv_expand_ptr = _wexpand_wild;
#endif
