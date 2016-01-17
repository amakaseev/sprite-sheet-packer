/*
 * opngcore/version.c
 * Version info.
 *
 * Copyright (C) 2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include "opngcore.h"

#include "png.h"
#include "zlib.h"

#define OPNGLIB_INTERNAL
#include "version.h"


/*
 * Conveniently hard-coded version numbers of dependent libraries.
 */
#define OPNGLIB_PNMIO_VERSION    "0.3"
#define OPNGLIB_MINITIFF_VERSION "0.1"
#define OPNGLIB_CEXCEPT_VERSION  "2.0.1-optipng"


/*
 * Returns a version info object.
 */
const struct opng_version_info *
opng_get_version_info(void)
{
    static int has_result = 0;
    static struct opng_version_info result[] =
    {
        { "opnglib",   OPNGLIB_VERSION          },
        { "opngreduc", NULL                     },
        { "libpng",    NULL                     },
        { "zlib",      NULL                     },
        { "pnmio",     OPNGLIB_PNMIO_VERSION    },
        { "minitiff",  OPNGLIB_MINITIFF_VERSION },
        { "cexcept",   OPNGLIB_CEXCEPT_VERSION  },
        { NULL,        NULL                     }
    };

    if (!has_result)
    {
        result[2].library_version = png_get_libpng_ver(NULL);
        result[3].library_version = zlibVersion();
        has_result = 1;
    }

    return result;
}
