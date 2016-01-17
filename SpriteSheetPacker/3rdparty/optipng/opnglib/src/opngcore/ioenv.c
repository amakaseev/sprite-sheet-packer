/*
 * opngcore/ioenv.c
 * I/O environment utilities.
 *
 * Copyright (C) 2010-2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include "opngcore.h"
#include "io.h"

#include <stdlib.h>
#include <string.h>

#define OPNGLIB_INTERNAL
#include "ioenv.h"
#include "util.h"


/*
 * The I/O environment structure.
 */
struct opng_ioenv
{
    const char *in_fname;
    const char *out_fname;
    const char *out_dirname;
    const char *bak_fname;
    char *out_fname_xbuf;
    char *outpng_fname_xbuf;
    char *bak_fname_xbuf;
    char out_fname_buf[OPNG_FNAME_MAX];
    char outpng_fname_buf[OPNG_FNAME_MAX];
    char bak_fname_buf[OPNG_FNAME_MAX];
    unsigned int flags;
    int overwrite_infile;
};


/*
 * Creates an I/O environment object.
 */
opng_ioenv_t *
opng_ioenv_create(const char *in_fname,
                  const char *out_fname,
                  const char *out_dirname,
                  unsigned int flags)
{
    opng_ioenv_t *ioenv;
    size_t path_len, chk_len;
    size_t xbuf_size;

    /* Allocate the result. */
    ioenv = (struct opng_ioenv *)opng_xmalloc(sizeof(struct opng_ioenv));
    memset(ioenv, 0, sizeof(struct opng_ioenv));
    ioenv->flags = flags;

    /* Set the input file name. */
    ioenv->in_fname = in_fname;

    /* Set the output file name. Change its directory component if required. */
    if (out_dirname != NULL)
    {
        OPNG_WEAK_ASSERT(!(ioenv->flags & OPNG_IOENV_USE_STDOUT),
                         "opng_ioenv_create: Changing directory of STDOUT?");
        path_len = optk_set_dirname(ioenv->out_fname_buf, OPNG_FNAME_MAX,
                                    out_fname, out_dirname);
        if (path_len < OPNG_FNAME_MAX)
            out_fname = ioenv->out_fname_buf;
        else
        {
            /* The resulting name does not fit in the pre-allocated buffer.
             * Allocate a larger buffer.
             */
            xbuf_size = path_len + 1;
            ioenv->out_fname_xbuf = (char *)opng_xmalloc(xbuf_size);
            chk_len = optk_set_dirname(ioenv->out_fname_xbuf, xbuf_size,
                                       out_fname, out_dirname);
            OPNG_ASSERT(path_len == chk_len,
                        "opng_ioenv_create: Malfunction in optk_set_dirname?");
            out_fname = ioenv->out_fname_xbuf;
        }
    }
    ioenv->out_fname = out_fname;
    ioenv->out_dirname = out_dirname;

    return ioenv;
}

/*
 * Initializes an I/O environment object.
 */
int
opng_ioenv_init(opng_ioenv_t *ioenv, const char *png_extname)
{
    const char *in_fname, *out_fname;
    const char *bak_fname, *bak_base_fname, *bak_extname;
    int overwrite_infile;
    size_t path_len, chk_len;
    size_t xbuf_size;

    in_fname = ioenv->in_fname;
    out_fname = ioenv->out_fname;
    bak_extname = "bak";

    /* Change the output file extension to ".png" if required. */
    if (png_extname != NULL)
    {
        OPNG_WEAK_ASSERT(!(ioenv->flags & OPNG_IOENV_USE_STDOUT),
                         "opng_ioenv_init: Changing directory of STDOUT?");
        path_len = optk_set_extname(ioenv->outpng_fname_buf, OPNG_FNAME_MAX,
                                    out_fname, 1, png_extname);
        if (path_len < OPNG_FNAME_MAX)
            out_fname = ioenv->outpng_fname_buf;
        else
        {
            /* The resulting name does not fit in the pre-allocated buffer. */
            xbuf_size = path_len + 1;
            ioenv->outpng_fname_xbuf = (char *)opng_xmalloc(xbuf_size);
            chk_len = optk_set_extname(ioenv->outpng_fname_xbuf, xbuf_size,
                                       out_fname, 1, png_extname);
            OPNG_ASSERT(path_len == chk_len,
                        "opng_ioenv_init: Malfunction in optk_set_extname?");
            out_fname = ioenv->outpng_fname_xbuf;
        }
        ioenv->out_fname = out_fname;
    }

    /* Set the overwrite_infile field. */
    if (ioenv->flags & (OPNG_IOENV_USE_STDIN | OPNG_IOENV_USE_STDOUT))
        overwrite_infile = 0;
    else
    {
        overwrite_infile = optk_test_eq(in_fname, out_fname);
        if (overwrite_infile < 0)
        {
            /* We don't know if the two paths point to the same file.
             * Use a crude path name comparison.
             */
            overwrite_infile = (strcmp(in_fname, out_fname) == 0) ? 1 : 0;
        }
    }
    ioenv->overwrite_infile = overwrite_infile;

    /* Check the output file. */
    if (ioenv->flags & OPNG_IOENV_USE_STDOUT)
        return 0;
    if (optk_test(out_fname, "e") <= 0)
        return 0;
    if (!overwrite_infile)
    {
        if (!(ioenv->flags & (OPNG_IOENV_BACKUP | OPNG_IOENV_OVERWRITE)))
        {
            opng_error(out_fname,
                       "The output file exists",
                       "Rerun the program using -backup");
            return -1;
        }
    }
    if (optk_test(out_fname, "fw") == 0)
    {
        opng_error(out_fname, "Can't write the output file", NULL);
        return -1;
    }

    /* Initialize the backup file name. */
    bak_base_fname = (overwrite_infile ? in_fname : out_fname);
    path_len = optk_set_extname(ioenv->bak_fname_buf, OPNG_FNAME_MAX,
                                bak_base_fname, 0, bak_extname);
    if (path_len < OPNG_FNAME_MAX)
        bak_fname = ioenv->bak_fname_buf;
    else
    {
        /* The resulting name does not fit in the pre-allocated buffer. */
        xbuf_size = path_len + 1;
        ioenv->bak_fname_xbuf = (char *)opng_xmalloc(xbuf_size);
        chk_len = optk_set_extname(ioenv->bak_fname_xbuf, xbuf_size,
                                   bak_base_fname, 0, bak_extname);
        OPNG_ASSERT(path_len == chk_len,
                    "opng_ioenv_init: Malfunction in optk_set_extname?");
        bak_fname = ioenv->bak_fname_xbuf;
    }
    ioenv->bak_fname = bak_fname;

    /* Check the backup file. */
    if (optk_test(bak_fname, "e") > 0)
    {
        if (ioenv->flags & OPNG_IOENV_OVERWRITE)
        {
            if (optk_test(bak_fname, "fw") == 0)
            {
                opng_error(bak_fname, "Can't overwrite the backup file", NULL);
                return -1;
            }
        }
        else
        {
            opng_error(out_fname,
                       "Can't back up the output file",
                       "Rerun the program using -overwrite");
            return -1;
        }
    }
    return 0;
}

/*
 * Prepares an I/O environment for processing.
 */
int
opng_ioenv_begin_output(opng_ioenv_t *ioenv)
{
    const char *in_fname = ioenv->in_fname;
    const char *out_fname = ioenv->out_fname;
    const char *bak_fname = ioenv->bak_fname;

    if (ioenv->flags & OPNG_IOENV_USE_STDOUT)
        return 0;

    /* Make room for the output file. */
    if (ioenv->overwrite_infile)
    {
        /* The output file will be written over the input file,
         * although their names may be different.
         * It is important to back up the input, rather than the output.
         */
        OPNG_ASSERT(bak_fname != NULL,
                    "opng_ioenv_begin_output: No backup file name");
        if (optk_rename(in_fname, bak_fname, 1) != 0)
        {
            opng_error(in_fname, "Can't back up the input file", NULL);
            return -1;
        }
    }
    else
    {
        if (!(ioenv->flags & OPNG_IOENV_BACKUP))
        {
            /* There will be no backup file, even if we already have a name. */
            bak_fname = ioenv->bak_fname = NULL;
        }
        if (optk_test(out_fname, "e") > 0)
        {
            if (bak_fname != NULL)
            {
                if (optk_rename(out_fname, bak_fname, 1) != 0)
                {
                    opng_error(out_fname,
                               "Can't back up the output file", NULL);
                    return -1;
                }
            }
        }
        else if (ioenv->out_dirname != NULL)
            optk_create_dir(ioenv->out_dirname);
    }
    return 0;
}

/*
 * Unrolls an I/O environment.
 */
int
opng_ioenv_unroll_output(opng_ioenv_t *ioenv)
{
    const char *in_fname = ioenv->in_fname;
    const char *out_fname = ioenv->out_fname;
    const char *bak_fname = ioenv->bak_fname;
    const char *orig_fname;

    if (ioenv->flags & OPNG_IOENV_USE_STDOUT)
        return 0;

    /* Remove the output file. */
    optk_unlink(out_fname);

    /* Restore the original file, if possible. */
    if (bak_fname != NULL)
    {
        orig_fname = (ioenv->overwrite_infile ? in_fname : out_fname);
        if (optk_rename(bak_fname, orig_fname, 0) != 0)
        {
            opng_warning(in_fname,
                         "Can't recover the original file from backup");
            return -1;
        }
    }
    return 0;
}

/*
 * Commits an I/O environment.
 */
int
opng_ioenv_end_output(opng_ioenv_t *ioenv)
{
    const char *in_fname = ioenv->in_fname;
    const char *out_fname = ioenv->out_fname;
    const char *bak_fname = ioenv->bak_fname;
    const char *orig_fname;

    if (ioenv->flags & OPNG_IOENV_USE_STDOUT)
        return 0;

    /* Copy file attributes (e.g. ownership, access rights, time stamps)
     * if requested.
     */
    if (ioenv->flags & OPNG_IOENV_PRESERVE)
    {
        orig_fname = (ioenv->overwrite_infile ? bak_fname : in_fname);
        optk_copy_attr(orig_fname, out_fname);
    }

    /* Remove the backup file if it was created but not requested. */
    if (bak_fname != NULL && !(ioenv->flags & OPNG_IOENV_BACKUP))
    {
        OPNG_WEAK_ASSERT(ioenv->overwrite_infile,
                         "opng_ioenv_end_output: Litter alert");
        if (optk_unlink(bak_fname) != 0)
        {
            opng_warning(bak_fname, "Can't remove the backup file");
            return -1;
        }
    }
    return 0;
}

/*
 * Retrieves the input file name.
 */
const char *
opng_ioenv_get_in_fname(opng_ioenv_t *ioenv)
{
    return ioenv->in_fname;
}

/*
 * Retrieves the output file name.
 */
const char *
opng_ioenv_get_out_fname(opng_ioenv_t *ioenv)
{
    return ioenv->out_fname;
}

/*
 * Retrieves the backup file name.
 */
const char *
opng_ioenv_get_bak_fname(opng_ioenv_t *ioenv)
{
    return ioenv->bak_fname;
}

/*
 * Queries whether the input file is overwritten.
 */
int
opng_ioenv_get_overwrite_infile(opng_ioenv_t *ioenv)
{
    return ioenv->overwrite_infile;
}

/*
 * Destroys an I/O environment object.
 */
void
opng_ioenv_destroy(opng_ioenv_t *ioenv)
{
    free(ioenv->out_fname_xbuf);
    free(ioenv->outpng_fname_xbuf);
    free(ioenv->bak_fname_xbuf);
    free(ioenv);
}
