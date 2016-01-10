/**
 * @file opnglib/opngcore.h
 *
 * @brief
 * OPNGCORE is a PNG Compression Optimization and Recovery Engine.
 *
 * Copyright (C) 2001-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file, or visit
 * http://www.opensource.org/licenses/zlib-license.php
 **/

#ifndef OPNGLIB_OPNGCORE_H
#define OPNGLIB_OPNGCORE_H

#include <limits.h>
#include <stddef.h>
#include "../optk/bits.h"


#ifdef __cplusplus
extern "C" {
#endif


/*** Types ***/

/**
 * The optimizer type.
 *
 * An optimizer object can optimize one or more images, sequentially.
 *
 * @bug
 * Multiple optimizer objects are designed to run in parallel
 * (but they don't, yet).
 **/
typedef struct opng_optimizer /*opaque*/ opng_optimizer_t;

/**
 * The image type.
 *
 * Image objects are currently used internally only.
 * There is no public API to access them yet.
 *
 * @todo
 * 3rd-party applications may wish to use this type in order to create
 * optimized PNG files from in-memory images (e.g. in-memory compressed
 * files, raw pixel data, BMP handles, etc.).
 * Your contribution will be appreciated ;-)
 **/
typedef struct opng_image /*opaque*/ opng_image_t;

/**
 * The user options structure.
 *
 * This is set via the command line by the optipng driver, and it may be
 * set by 3rd-party applications in order to customize PNG optimization.
 **/
struct opng_options
{
    /* general options */
    int backup;
    int out;
    int fix;
    int force;
    int no_clobber;
    int no_create;
    int preserve;
    int snip;
    int use_stdin;
    int use_stdout;
    int verbose;

    /* optimization options */
    int interlace;
    int nb, nc, nm, np, nz;
    int optim_level;
    int paranoid;
    optk_bits_t filter_set;
    optk_bits_t zcompr_level_set;
    optk_bits_t zmem_level_set;
    optk_bits_t zstrategy_set;
    int zwindow_bits;
};


/*** Optimizer ***/

/**
 * Creates an optimizer object.
 * @return the optimizer created, or @c NULL on failure.
 **/
opng_optimizer_t *
opng_create_optimizer(void);

/**
 * Sets the user options in an optimizer object.
 * An optimizer can run only if it has been given a set of options;
 * there are no defaults.
 * Options can be changed repeatedly between optimization sessions
 * (for the benefit of interactive applications), but they shall
 * not be changed @em during these sessions.
 * @param optimizer
 *        the optimizer object whose user options are changed.
 * @param user_options
 *        the user options.
 * @return 0 if the options are valid, or -1 otherwise.
 **/
int
opng_set_options(opng_optimizer_t *optimizer,
                 const struct opng_options *user_options);

/**
 * Optimizes an image file.
 * @param optimizer
 *        the optimizer object.
 * @param in_fname
 *        the input file name.
 * @param out_fname
 *        the output file name.
 *        It can be @c NULL, in which case the output has the same name
 *        as the input.
 * @param out_dirname
 *        the output directory name.
 *        It can be @c NULL.
 * @return 0 on success, or a non-zero exit code on failure.
 **/
int
opng_optimize_file(opng_optimizer_t *optimizer,
                   const char *in_fname,
                   const char *out_fname,
                   const char *out_dirname);

#if 0  /* not implemented */
/**
 * Optimizes an image object.
 * @param optimizer
 *        the optimizer object.
 * @param image
 *        the image to be optimized.
 * @param out_fname
 *        the output file name.
 * @return 0 on success, or a non-zero exit code on failure.
 **/
int
opng_optimize_image(opng_optimizer_t *optimizer,
                    const opng_image_t image,
                    const char *out_fname);
#endif

/**
 * Destroys an optimizer object.
 * @param optimizer
 *        the optimizer object to be destroyed.
 **/
void
opng_destroy_optimizer(opng_optimizer_t *optimizer);


/*** Logging ***/

/**
 * Message severity levels.
 * Each level allows up to 10 custom sub-levels. The level numbers
 * are loosely based on the Python logging module.
 **/
enum
{
    OPNG_MSG_ALL      = 0,
    OPNG_MSG_DEBUG    = 10,
    OPNG_MSG_INFO     = 20,
    OPNG_MSG_WARNING  = 30,
    OPNG_MSG_ERROR    = 40,
    OPNG_MSG_CRITICAL = 50,
    OPNG_MSG_OFF      = (unsigned int)(-1),
    OPNG_MSG_DEFAULT  = OPNG_MSG_OFF
};

/**
 * Message formats.
 **/
enum
{
    OPNG_MSGFMT_RAW     = 0, /* do not format messages, just print them raw */
    OPNG_MSGFMT_UNIX    = 1, /* format messages in Unix-ish style */
    OPNG_MSGFMT_FANCY   = 2, /* format messages in a more eye-catching style */
    OPNG_MSGFMT_DEFAULT = OPNG_MSGFMT_UNIX
};

/**
 * Sets the program name associated with the logger.
 * @param program_name
 *        the program name displayed on warnings or errors.
 *        It can be @c NULL.
 **/
void
opng_set_logging_name(const char *program_name);

/**
 * Sets the logging severity level.
 * @param level
 *        the severity level @c OPNG_MSG_...
 * @see opng_print_message.
 **/
void
opng_set_logging_level(unsigned int level);

/**
 * Sets the logging format.
 * @param format
 *        the format @c OPNG_MSGFMT_...
 * @see opng_print_message.
 **/
void
opng_set_logging_format(int format);

/**
 * Flushes the logger at the given severity level.
 * To avoid useless fflush operations, it is recommended to set the severity
 * level to the level of the messages that have been displayed and need to be
 * flushed.
 * @param level
 *        the minimum severity level at which flushing is triggered.
 **/
int
opng_flush_logging(unsigned int level);

/**
 * Prints a message of a given severity level, concerning a given file name,
 * using the current logging message format, to the logger.
 * @param fname
 *        the file name associated with the message; can be @c NULL.
 * @param level
 *        the severity level @c OPNG_MSG_...
 * @param message
 *        the message to be displayed.
 * @return 1 if a message was displayed, 0 if nothing was displayed,
 *         or -1 if an error occurred.
 **/
int
opng_print_message(unsigned int level, const char *fname, const char *message);

/**
 * Prints a printf-formatted debug message (@c level = @c OPNG_MSG_DEBUG)
 * to the logger.
 **/
int
opng_debugf(const char *format, ...);

/**
 * Prints a printf-formatted informational message (@c level = @c OPNG_MSG_INFO)
 * to the logger.
 **/
int
opng_printf(const char *format, ...);

/**
 * Prints a warning message (@c level = @c OPNG_MSG_WARNING) to the logger.
 **/
int
opng_warning(const char *fname, const char *message);

/**
 * Prints an error message (@c level = @c OPNG_MSG_ERROR), optionally
 * accompanied by a submessage, to the logger.
 **/
int
opng_error(const char *fname, const char *message, const char *submessage);


/*** PNG encoding ***/

/**
 * Encoder constants and limits.
 **/
enum
{
    OPNG_OPTIM_LEVEL_MIN       = -2,
    OPNG_OPTIM_LEVEL_MAX       =  6,
    OPNG_OPTIM_LEVEL_FASTEST   = -2,
    OPNG_OPTIM_LEVEL_FAST      = -1,
    OPNG_OPTIM_LEVEL_DEFAULT   =  2,

    OPNG_FILTER_MIN            = 0,
    OPNG_FILTER_MAX            = 5,
    OPNG_FILTER_SET_MASK       = (1 << (5+1)) - (1 << 0)  /* 0x003f */,

    OPNG_ZCOMPR_LEVEL_MIN      = 1,
    OPNG_ZCOMPR_LEVEL_MAX      = 9,
    OPNG_ZCOMPR_LEVEL_SET_MASK = (1 << (9+1)) - (1 << 1)  /* 0x03fe */,

    OPNG_ZMEM_LEVEL_MIN        = 1,
    OPNG_ZMEM_LEVEL_MAX        = 9,
    OPNG_ZMEM_LEVEL_SET_MASK   = (1 << (9+1)) - (1 << 1)  /* 0x03fe */,

    OPNG_ZSTRATEGY_MIN         = 0,
    OPNG_ZSTRATEGY_MAX         = 3,
    OPNG_ZSTRATEGY_SET_MASK    = (1 << (3+1)) - (1 << 0)  /* 0x000f */,

    /* Note:
     * There are no -zw iterations. They appear to offer no advantage over -zc.
     */
    OPNG_ZWINDOW_BITS_MIN      = 8,
    OPNG_ZWINDOW_BITS_MAX      = 15,
    OPNG_ZWINDOW_BITS_SET_MASK = (1 << (15+1)) - (1 << 8) /* 0xff00 */
};


/*** Miscellanea ***/

/**
 * The version info structure.
 **/
struct opng_version_info
{
    const char *library_name;
    const char *library_version;
};

/**
 * Returns an array of version info objects, terminated by NULLs.
 **/
const struct opng_version_info *
opng_get_version_info(void);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPNGLIB_OPNGCORE_H */
