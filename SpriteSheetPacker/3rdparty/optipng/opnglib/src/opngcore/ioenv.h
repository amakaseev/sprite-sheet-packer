/*
 * opngcore/ioenv.h
 * I/O environment utilities.
 *
 * Copyright (C) 2010-2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef OPNGCORE_IOENV_H
#define OPNGCORE_IOENV_H

#ifndef OPNGLIB_INTERNAL
#error This header, internal to opnglib, is not meant to be visible outside.
#endif

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 * The portabilized FILENAME_MAX.
 */
#ifdef FILENAME_MAX
#define OPNG_FNAME_MAX FILENAME_MAX
#else
#define OPNG_FNAME_MAX 256
#endif


/*
 * The I/O environment object.
 */
typedef struct opng_ioenv /*opaque*/ opng_ioenv_t;


/*
 * I/O environment flags.
 */
#define OPNG_IOENV_USE_STDIN  0x0001
#define OPNG_IOENV_USE_STDOUT 0x0002
#define OPNG_IOENV_BACKUP     0x0004
#define OPNG_IOENV_OVERWRITE  0x0008
#define OPNG_IOENV_PRESERVE   0x0010


/*
 * Creates an I/O environment object.
 */
opng_ioenv_t *
opng_ioenv_create(const char *in_fname,
                  const char *out_fname,
                  const char *out_dirname,
                  unsigned int flags);

/*
 * Initializes an I/O environment object.
 */
int
opng_ioenv_init(opng_ioenv_t *ioenv, const char *png_extname);

/*
 * Prepares an I/O environment for processing.
 */
int
opng_ioenv_begin_output(opng_ioenv_t *ioenv);

/*
 * Unrolls an I/O environment.
 */
int
opng_ioenv_unroll_output(opng_ioenv_t *ioenv);

/*
 * Commits an I/O environment.
 */
int
opng_ioenv_end_output(opng_ioenv_t *ioenv);

/*
 * Retrieves the input file name.
 */
const char *
opng_ioenv_get_in_fname(opng_ioenv_t *ioenv);

/*
 * Retrieves the output file name.
 */
const char *
opng_ioenv_get_out_fname(opng_ioenv_t *ioenv);

/*
 * Retrieves the backup file name.
 */
const char *
opng_ioenv_get_bak_fname(opng_ioenv_t *ioenv);

/*
 * Queries whether the input file is overwritten.
 */
int
opng_ioenv_get_overwrite_infile(opng_ioenv_t *ioenv);

/*
 * Destroys an I/O environment object.
 */
void
opng_ioenv_destroy(opng_ioenv_t *ioenv);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPNGCORE_IOENV_H */
