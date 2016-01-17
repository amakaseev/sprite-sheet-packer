/*
 * opngcore/optim.c
 * The main PNG optimization engine.
 *
 * Copyright (C) 2001-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include "opngcore.h"
#include "opngtrans.h"
#include "bits.h"
#include "io.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opngreduc.h"
#include "png.h"
#include "pngxtern.h"
#include "pngxutil.h"
#include "zlib.h"

#define OPNGLIB_INTERNAL
#include "codec.h"
#include "image.h"
#include "ioenv.h"
#include "util.h"


/*
 * Optimization presets
 */
static const char *
filter_presets[OPNG_OPTIM_LEVEL_MAX - OPNG_OPTIM_LEVEL_MIN + 1] =
    { "", "", NULL, "", "0,5", "0,5", "0-5", "0-5", "0-5" };
static const char *
zcompr_level_presets[OPNG_OPTIM_LEVEL_MAX - OPNG_OPTIM_LEVEL_MIN + 1] =
    { "3", "9", NULL, "9", "9", "9", "9", "3-9", "1-9" };
static const char *
zmem_level_presets[OPNG_OPTIM_LEVEL_MAX - OPNG_OPTIM_LEVEL_MIN + 1] =
    { "", "", NULL, "", "", "8-9", "8-9", "8-9", "7-9" };
static const char *
zstrategy_presets[OPNG_OPTIM_LEVEL_MAX - OPNG_OPTIM_LEVEL_MIN + 1] =
    { "", "", NULL, "", "0-3", "0-3", "0-3", "0-3", "0-3" };


/*
 * The optimization engine
 */
struct opng_optimizer
{
    const opng_transformer_t *transformer;
    struct opng_options options;
    unsigned int file_count;
    unsigned int err_count;
    unsigned int fix_count;
};

/*
 * The optimization session structure
 */
struct opng_session
{
    const struct opng_options *options;
    const opng_transformer_t *transformer;
    struct opng_image image;
    struct opng_encoding_stats in_stats;
    struct opng_encoding_stats out_stats;
    const char *in_fname;
    const char *out_fname;
    optk_fsize_t best_idat_size;
    optk_fsize_t max_idat_size;
    png_uint_32 flags;
    optk_bits_t filter_set;
    optk_bits_t zcompr_level_set;
    optk_bits_t zmem_level_set;
    optk_bits_t zstrategy_set;
    struct opng_encoding_params best_params;
    int num_iterations;
};


/*
 * Prints, in a compact form, the ratio between two numbers.
 */
static void
opng_print_fsize_ratio(optk_fsize_t num, optk_fsize_t denom, int force_percent)
{
    /* (1) num/denom = 0/0                  ==> print "??%"
     * (2) num/denom = INFINITY             ==> print "INFTY%"
     * (3) 0 <= num/denom < 99.995%         ==> use the percent format "99.99%"
     *     if force_percent:
     * (4)    0.995 <= num/denom < INFINITY ==> use the percent format "999%"
     *     else:
     * (5)    0.995 <= num/denom < 99.995   ==> use the factor format "9.99x"
     * (6)    99.5 <= num/denom < INFINITY  ==> use the factor format "999x"
     *     end if
     */

    optk_fsize_t integer_part, remainder;
    unsigned int fractional_part, scale;
    long double scaled_ratio;

    /* (1,2): num/denom = 0/0 or num/denom = INFINITY */
    if (denom == 0)
    {
        opng_printf(num == 0 ? "??%%" : "INFTY%%");
        return;
    }

    /* (3): 0 <= num/denom < 99.995% */
    /* num/denom < 99.995% <==> denom/(denom-num) < 20000 */
    if (num < denom && denom / (denom - num) < 20000)
    {
        scale = 10000;
        scaled_ratio =
            ((long double)num * (long double)scale) / (long double)denom;
        fractional_part = (unsigned int)(scaled_ratio + 0.5);
        /* Adjust the scaled result in the event of a roundoff error. */
        /* Such error may occur only if the numerator is extremely large. */
        if (fractional_part >= scale)
            fractional_part = scale - 1;
        opng_printf("%u.%02u%%", fractional_part / 100, fractional_part % 100);
        return;
    }

    /* Extract the integer part out of the fraction for the remaining cases. */
    integer_part = num / denom;
    remainder = num % denom;
    scale = 100;
    scaled_ratio = ((double)remainder * (double)scale) / (double)denom;
    fractional_part = (unsigned int)(scaled_ratio + 0.5);
    if (fractional_part >= scale)
    {
        fractional_part = 0;
        ++integer_part;
    }

    /* (4): 0.995 <= num/denom < INFINITY */
    if (force_percent)
    {
        opng_printf("%"OPTK_FSIZE_PRIu"%02u%%", integer_part, fractional_part);
        return;
    }

    /* (5): 0.995 <= num/denom < 99.995 */
    if (integer_part < 100)
    {
        opng_printf("%"OPTK_FSIZE_PRIu".%02ux", integer_part, fractional_part);
        return;
    }

    /* (6): 99.5 <= num/denom < INFINITY */
    /* Round to the nearest integer. */
    /* Recalculate the integer part, for corner cases like 123.999. */
    integer_part = num / denom;
    if (remainder >= (denom + 1) / 2)
        ++integer_part;
    opng_printf("%"OPTK_FSIZE_PRIu"x", integer_part);
}

/*
 * Prints, in a descriptive form, the difference between two sizes.
 */
static void
opng_print_fsize_difference(optk_fsize_t init_size, optk_fsize_t final_size,
                            int show_ratio)
{
    optk_fsize_t difference;
    int sign;

    if (init_size <= final_size)
    {
        sign = 0;
        difference = final_size - init_size;
    }
    else
    {
        sign = 1;
        difference = init_size - final_size;
    }

    if (difference == 0)
    {
        opng_printf("no change");
        return;
    }
    if (difference == 1)
        opng_printf("1 byte");
    else
        opng_printf("%"OPTK_FSIZE_PRIu" bytes", difference);
    if (show_ratio && init_size > 0)
    {
        opng_printf(" = ");
        opng_print_fsize_ratio(difference, init_size, 0);
    }
    opng_printf(sign == 0 ? " increase" : " decrease");
}

/*
 * Prints a line of information regarding the image format.
 */
static void
opng_print_image_format_line(const struct opng_image *image,
                             const char *format_name, const char *format_xinfo)
{
    const char *interlaced_add_str, *format_add_str1, *format_add_str2;

    /* Print the image dimensions and format information. */
    if (image->interlace_type != PNG_INTERLACE_NONE)
        interlaced_add_str = ", interlaced";
    else
        interlaced_add_str = "";
    if (format_xinfo != NULL)
    {
        format_add_str1 = ", ";
        format_add_str2 = format_xinfo;
    }
    else
        format_add_str1 = format_add_str2 = "";
    opng_printf("   %"OPTK_PRIu32"x%"OPTK_PRIu32" pixels, %s format%s%s%s\n",
                (optk_uint32_t)image->width,
                (optk_uint32_t)image->height,
                format_name,
                interlaced_add_str,
                format_add_str1,
                format_add_str2);
}

/*
 * Prints a line of information regarding the image representation.
 */
static void
opng_print_image_info_line(const struct opng_image *image)
{
    static const int type_channels[8] = {1, 0, 3, 1, 2, 0, 4, 0};
    int channels;

    /* Print the channel depth and the bit depth. */
    channels = type_channels[image->color_type & 7];
    if (channels != 1)
        opng_printf("   %dx%d bits/pixel", channels, image->bit_depth);
    else if (image->bit_depth != 1)
        opng_printf("   %d bits/pixel", image->bit_depth);
    else
        opng_printf("   1 bit/pixel");

    /* Print the color information and the image type. */
    if (image->color_type & PNG_COLOR_MASK_PALETTE)
    {
        if (image->num_palette == 1)
            opng_printf(", 1 color");
        else
            opng_printf(", %d colors", image->num_palette);
        if (image->num_trans > 0)
            opng_printf(" (%d transparent)", image->num_trans);
        opng_printf(" in palette\n");
    }
    else
    {
        opng_printf((image->color_type & PNG_COLOR_MASK_COLOR) ?
                    ", RGB" : ", grayscale");
        if (image->color_type & PNG_COLOR_MASK_ALPHA)
            opng_printf("+alpha");
        else if (image->trans_color_ptr != NULL)
            opng_printf("+transparency");
        opng_printf("\n");
    }
}

/*
 * Prints a partial line of information regarding the encoding parameters.
 */
static void
opng_print_encoding_line_begin(const struct opng_encoding_params *params)
{
    /* Print a 32-char line. */
    opng_printf("   zc = %d  zm = %d  zs = %d  f = %d",
                params->zcompr_level,
                params->zmem_level,
                params->zstrategy,
                params->filter);
    /* Flush the logger, because there will probably be a long wait. */
    opng_flush_logging(OPNG_MSG_INFO);
}

/*
 * Erases the line in which encoding parameters were printed.
 */
static void
opng_print_erase_encoding_line_begin()
{
    /* Erase a 32-char line. */
    opng_printf("\r%-32s\r", "");
    /* TODO: Make this less fragile. */
}

/*
 * Prints the IDAT size line.
 */
static void
opng_print_idat_size_line(optk_fsize_t idat_size)
{
    opng_printf("   IDAT size = %"OPTK_FSIZE_PRIu" bytes\n", idat_size);
}

/*
 * Prints the IDAT size line.
 */
static void
opng_print_idat_size_line_end(optk_fsize_t idat_size)
{
    opng_printf("\tIDAT size = %"OPTK_FSIZE_PRIu"\n", idat_size);
}

/*
 * Prints the IDAT size difference line.
 */
static void
opng_print_idat_size_difference_line(optk_fsize_t init_idat_size,
                                     optk_fsize_t final_idat_size)
{
    opng_printf("   IDAT size = %"OPTK_FSIZE_PRIu" bytes", final_idat_size);
    if (init_idat_size > 0)
    {
        opng_printf(" (");
        opng_print_fsize_difference(init_idat_size, final_idat_size, 0);
        opng_printf(")\n");
    }
    else
        opng_printf("\n");
}

/*
 * Prints the file size line.
 */
static void
opng_print_file_size_line(optk_fsize_t file_size)
{
    opng_printf("   file size = %"OPTK_FSIZE_PRIu" bytes\n", file_size);
}

/*
 * Prints the file size difference line.
 */
static void
opng_print_file_size_difference_line(optk_fsize_t init_file_size,
                                     optk_fsize_t final_file_size)
{
    opng_printf("   file size = %"OPTK_FSIZE_PRIu" bytes (", final_file_size);
    opng_print_fsize_difference(init_file_size, final_file_size, 1);
    opng_printf(")\n");
}

/*
 * Prints the status report.
 */
static void
opng_print_status_report(const struct opng_optimizer *optimizer)
{
    opng_printf("Status report:\n");
    opng_printf("   %u file(s) have been processed.\n",
                optimizer->file_count);
    if (optimizer->err_count > 0)
    {
        opng_printf("   %u erroneous file(s) have been encountered.\n",
                    optimizer->err_count);
        if (optimizer->fix_count > 0)
            opng_printf("   %u erroneous file(s) have been fixed.\n",
                        optimizer->fix_count);
    }
}

/*
 * Reads an image from an image file stream.
 * Reduces the image if possible.
 */
static int
opng_read_file(struct opng_session *session, FILE *stream)
{
    struct opng_codec_context context;
    const struct opng_options *options;
    struct opng_image *image;
    struct opng_encoding_stats *stats;
    const char *format_name;
    const char *format_xdesc;
    int reductions;

    options = session->options;
    image = &session->image;
    stats = &session->in_stats;
    opng_init_codec_context(&context,
                            image,
                            stats,
                            0,
                            session->transformer);
    if (opng_decode_image(&context, stream, session->in_fname,
                          &format_name, &format_xdesc) < 0)
    {
        opng_decode_finish(&context, 1);
        return -1;
    }

    /* Display the input image file information. */
    opng_print_image_format_line(image, format_name, format_xdesc);
    opng_print_image_info_line(image);
    if (stats->flags & OPNG_HAS_PNG_DATASTREAM)
    {
        opng_print_idat_size_line(stats->idat_size);
        OPNG_WEAK_ASSERT(stats->idat_size != 0, "IDAT not found inside PNG");
    }
    else
        OPNG_WEAK_ASSERT(stats->idat_size == 0, "IDAT found outside PNG");
    opng_print_file_size_line(stats->file_size);

    if (stats->flags & OPNG_HAS_SNIPPED_IMAGES)
    {
        opng_printf("Snipping:\n");
        stats->flags |= OPNG_HAS_JUNK;
    }

    if (stats->flags & OPNG_HAS_STRIPPED_METADATA)
    {
        opng_printf("Stripping metadata:\n");
        stats->flags |= OPNG_HAS_JUNK;
    }

    /* Set/reset image data objects, if applicable.
     * This operation must be done before reductions.
     */
    if (opng_decode_transform_image(&context))
    {
        opng_printf("Transforming:\n");
        /* Recompression is mandatory after a successful transformation. */
        stats->flags |= OPNG_NEEDS_NEW_IDAT;
    }

    /* Choose the applicable image reductions. */
    reductions = OPNG_REDUCE_ALL;
    if (options->nb)
        reductions &= ~OPNG_REDUCE_BIT_DEPTH;
    if (options->nc)
        reductions &= ~OPNG_REDUCE_COLOR_TYPE;
    if (options->np)
        reductions &= ~OPNG_REDUCE_PALETTE;
    if (options->nz && (stats->flags & OPNG_HAS_PNG_DATASTREAM))
    {
        /* Do not reduce files with PNG datastreams under -nz. */
        reductions = OPNG_REDUCE_NONE;
    }
    if (stats->flags & OPNG_HAS_DIGITAL_SIGNATURE)
    {
        /* Do not reduce signed files. */
        reductions = OPNG_REDUCE_NONE;
    }
    if ((stats->flags & OPNG_IS_PNG_FILE) &&
        (stats->flags & OPNG_HAS_MULTIPLE_IMAGES) &&
        (reductions != OPNG_REDUCE_NONE))
    {
        opng_warning(session->in_fname,
            "Can't reliably reduce APNG file; disabling reductions");
        opng_print_message(OPNG_MSG_INFO, NULL,
            "(Did you want to -snip and optimize the first frame?)");
        reductions = OPNG_REDUCE_NONE;
    }

    /* Try to reduce the image. */
    reductions = opng_decode_reduce_image(&context, reductions);
    if (reductions != OPNG_REDUCE_NONE)
    {
        opng_printf("Reducing:\n");
        opng_print_image_info_line(image);
    }
    if (reductions < 0)
    {
        opng_error(session->in_fname,
            "An unexpected error occurred while reducing the image", NULL);
        opng_decode_finish(&context, 1);
        return -1;
    }

    /* Change the interlace type if requested.
     * Such a change enforces IDAT recoding.
     */
    if (options->interlace >= 0 && image->interlace_type != options->interlace)
    {
        image->interlace_type = options->interlace;
        stats->flags |= OPNG_NEEDS_NEW_IDAT;
    }

    /* Keep the loaded image data. */
    opng_decode_finish(&context, 0);
    return 0;
}

/*
 * Writes an image to a PNG file stream.
 * Performs the encoding without any I/O, if the given stream is NULL.
 */
static int
opng_write_file(struct opng_session *session,
                const struct opng_encoding_params *params,
                FILE *stream)
{
    struct opng_codec_context context;
    const char *fname;
    optk_fsize_t expected_idat_size;
    int result;

    if (stream == NULL)
    {
        /* This is a trial. No file is actually written.
         * Use the input file name for error reporting.
         */
        fname = session->in_fname;
        expected_idat_size = session->max_idat_size;
    }
    else
    {
        fname = session->out_fname;
        expected_idat_size = session->best_idat_size;
    }

    opng_init_codec_context(&context,
                            &session->image,
                            &session->out_stats,
                            expected_idat_size,
                            session->transformer);
    result = opng_encode_image(&context, params, stream, fname);
    opng_encode_finish(&context);
    return result;
}

/*
 * PNG file copying
 */
static int
opng_copy_file(struct opng_session *session, FILE *in_stream, FILE *out_stream)
{
    struct opng_codec_context context;

    opng_init_codec_context(&context,
                            NULL,
                            &session->out_stats,
                            session->best_idat_size,
                            session->transformer);
    return opng_copy_png(&context,
                         in_stream, session->in_fname,
                         out_stream, session->out_fname);
}

/*
 * Iteration initialization
 */
static void
opng_init_iterations(struct opng_session *session)
{
    const struct opng_options *options;
    optk_bits_t filter_set, zcompr_level_set, zmem_level_set, zstrategy_set;
    int filtering_recommended;
    int t1, t2;

    options = session->options;

    /* Set the IDAT size limit. The trials that pass this limit will be
     * abandoned, as there will be no need to wait until their completion.
     * This limit may further decrease as iterations go on.
     */
    if ((session->flags & OPNG_NEEDS_NEW_IDAT) || options->paranoid)
       session->max_idat_size = OPNG_IDAT_SIZE_MAX;
    else
    {
        OPNG_ASSERT(session->in_stats.idat_size > 0, "No IDAT in input");
        /* Add the input PLTE and tRNS sizes to the initial max IDAT size,
         * to account for the changes that may occur during reduction.
         * This incurs a negligible overhead on processing only: the final
         * IDAT size will not be affected, because a precise check will be
         * performed at the end, inside opng_finish_iterations().
         */
        session->max_idat_size =
            session->in_stats.idat_size + session->in_stats.plte_trns_size;
    }

    /* Initialize the iteration sets. */
    filter_set = options->filter_set;
    zcompr_level_set = options->zcompr_level_set;
    zmem_level_set = options->zmem_level_set;
    zstrategy_set = options->zstrategy_set;

    /* Replace the empty sets with the libpng's "best guess" heuristics. */
    filtering_recommended =
        (session->image.bit_depth >= 8) &&
        !(session->image.color_type & PNG_COLOR_MASK_PALETTE);
    if (filter_set == 0)
    {
        /* Set -f0 or -f5 by default. */
        optk_bits_set(&filter_set, filtering_recommended ? 5 : 0);
    }
    if (filtering_recommended &&
        !optk_bits_test_any_in_range(filter_set, 1, 5))
    {
        /* This is a user-defined unfiltered-only setting.
         * Drop the recommendation. The user probably knows better.
         */
        filtering_recommended = 0;
    }
    if (zcompr_level_set == 0)
    {
        /* Set -zc9 by default. */
        optk_bits_set(&zcompr_level_set, 9);
    }
    if (zmem_level_set == 0)
    {
        /* Set -zm8 or -zm9 by default. */
        optk_bits_set(&zmem_level_set, filtering_recommended ? 9 : 8);
    }
    if (zstrategy_set == 0)
    {
        /* Set -zs0 or -zs1 by default. */
        optk_bits_set(&zstrategy_set, filtering_recommended ? 1 : 0);
    }

    /* Store the results into the session info. */
    session->filter_set = filter_set;
    session->zcompr_level_set = zcompr_level_set;
    session->zmem_level_set = zmem_level_set;
    session->zstrategy_set = zstrategy_set;
    t1 = optk_bits_count(zcompr_level_set) *
         optk_bits_count(zstrategy_set & ~((1 << Z_HUFFMAN_ONLY) | (1 << Z_RLE)));
    t2 = optk_bits_count(zstrategy_set & ((1 << Z_HUFFMAN_ONLY) | (1 << Z_RLE)));
    session->num_iterations = (t1 + t2) *
         optk_bits_count(zmem_level_set) * optk_bits_count(filter_set);
    OPNG_ASSERT(session->num_iterations > 0, "Invalid iteration parameters");
}

/*
 * Iteration
 */
static void
opng_iterate(struct opng_session *session)
{
    const struct opng_options *options;
    optk_bits_t filter_set, zcompr_level_set, zmem_level_set, zstrategy_set;
    optk_bits_t saved_zcompr_level_set;
    struct opng_encoding_params params;
    int filter, zcompr_level, zmem_level, zstrategy;
    optk_fsize_t out_idat_size;
    int counter;
    int line_reused;

    options = session->options;

    OPNG_ASSERT(session->num_iterations > 0, "Iterations not initialized");
    if ((session->num_iterations == 1) &&
        (session->flags & OPNG_NEEDS_NEW_IDAT))
    {
        /* We already know this combination will be selected.
         * Do not waste time running it twice.
         */
        params.filter = optk_bits_find_first(session->filter_set);
        params.zcompr_level = optk_bits_find_first(session->zcompr_level_set);
        params.zmem_level = optk_bits_find_first(session->zmem_level_set);
        params.zstrategy = optk_bits_find_first(session->zstrategy_set);
        params.zwindow_bits = options->zwindow_bits;
        session->best_params = params;
        session->best_idat_size = 0;
        return;
    }

    /* Prepare for the big iteration. */
    filter_set = session->filter_set;
    zcompr_level_set = session->zcompr_level_set;
    zmem_level_set = session->zmem_level_set;
    zstrategy_set = session->zstrategy_set;
    params.filter = -1;
    params.zcompr_level = -1;
    params.zmem_level = -1;
    params.zstrategy = -1;
    params.zwindow_bits = options->zwindow_bits;
    session->best_params = params;
    session->best_idat_size = OPNG_IDAT_SIZE_MAX + 1;

    /* Iterate through the "hyper-rectangle" (zc, zm, zs, f). */
    if (session->num_iterations == 1)
        opng_printf("Trying: 1 combination\n");
    else
        opng_printf("Trying: %d combinations\n", session->num_iterations);
    line_reused = 0;
    counter = 0;
    for (filter = OPNG_FILTER_MIN;
         filter <= OPNG_FILTER_MAX; ++filter)
    {
       if (optk_bits_test(filter_set, filter))
       {
          for (zstrategy = OPNG_ZSTRATEGY_MIN;
               zstrategy <= OPNG_ZSTRATEGY_MAX; ++zstrategy)
          {
             if (optk_bits_test(zstrategy_set, zstrategy))
             {
                /* The compression level has no significance under
                 * Z_HUFFMAN_ONLY or Z_RLE.
                 */
                saved_zcompr_level_set = zcompr_level_set;
                if (zstrategy == Z_HUFFMAN_ONLY)
                {
                   zcompr_level_set = 0;
                   optk_bits_set(&zcompr_level_set, 1);  /* use deflate_fast */
                }
                else if (zstrategy == Z_RLE)
                {
                   zcompr_level_set = 0;
                   optk_bits_set(&zcompr_level_set, 9);  /* use deflate_slow */
                }
                for (zcompr_level = OPNG_ZCOMPR_LEVEL_MAX;
                     zcompr_level >= OPNG_ZCOMPR_LEVEL_MIN; --zcompr_level)
                {
                   if (optk_bits_test(zcompr_level_set, zcompr_level))
                   {
                      for (zmem_level = OPNG_ZMEM_LEVEL_MAX;
                           zmem_level >= OPNG_ZMEM_LEVEL_MIN; --zmem_level)
                      {
                         if (optk_bits_test(zmem_level_set, zmem_level))
                         {
                            ++counter;
                            params.filter = filter;
                            params.zcompr_level = zcompr_level;
                            params.zmem_level = zmem_level;
                            params.zstrategy = zstrategy;
                            /* Leave params.zwindow_bits intact. */
                            opng_print_encoding_line_begin(&params);
                            opng_write_file(session, &params, NULL);
                            if (session->out_stats.idat_size > OPNG_IDAT_SIZE_MAX)
                            {
                               if (options->verbose)
                               {
                                  opng_printf("\tIDAT too big\n");
                                  line_reused = 0;
                               }
                               else
                               {
                                  opng_printf("\r");
                                  line_reused = 1;
                               }
                               continue;
                            }
                            out_idat_size = session->out_stats.idat_size;
                            opng_print_idat_size_line_end(out_idat_size);
                            line_reused = 0;
                            if (session->best_idat_size < out_idat_size)
                               continue;  /* it's bigger */
                            if (session->best_idat_size == out_idat_size &&
                                session->best_params.zstrategy >= Z_HUFFMAN_ONLY)
                               continue;  /* it's neither smaller nor faster */
                            session->best_params = params;
                            session->best_idat_size = out_idat_size;
                            if (!options->paranoid)
                               session->max_idat_size = out_idat_size;
                         }
                      }
                   }
                }
                zcompr_level_set = saved_zcompr_level_set;
             }
          }
       }
    }

    if (line_reused)
        opng_print_erase_encoding_line_begin();
    OPNG_ASSERT(counter == session->num_iterations,
                "Inconsistent iteration counter");
}

/*
 * Iteration finalization
 */
static void
opng_finish_iterations(struct opng_session *session)
{
    if (session->best_idat_size + session->out_stats.plte_trns_size <
        session->in_stats.idat_size + session->in_stats.plte_trns_size)
       session->flags |= OPNG_NEEDS_NEW_IDAT;
    if (session->flags & OPNG_NEEDS_NEW_IDAT)
    {
        opng_printf("Encoding:\n");
        opng_print_encoding_line_begin(&session->best_params);
        if (session->best_idat_size > 0)
            opng_print_idat_size_line_end(session->best_idat_size);
        else
            opng_printf("\n");
    }
}

/*
 * Image file optimization
 */
static int
opng_optimize_impl(struct opng_session *session, opng_ioenv_t *ioenv)
{
    const struct opng_options *options;
    FILE *in_stream;
    FILE *out_stream;
    int result;

    session->in_fname = opng_ioenv_get_in_fname(ioenv);
    opng_printf("Processing: %s\n", session->in_fname);
    in_stream = fopen(session->in_fname, "rb");
    if (in_stream == NULL)
    {
        opng_error(session->in_fname, "Can't open file", NULL);
        return -1;
    }

    result = opng_read_file(session, in_stream);
    fclose(in_stream);
    if (result < 0)
        return result;

    options = session->options;
    session->flags = session->in_stats.flags;
    if (options->force)
        session->flags |= OPNG_NEEDS_NEW_IDAT;

    /* Check the error flag. This must be the first check. */
    if (session->flags & OPNG_HAS_ERRORS)
    {
        if (options->fix)
        {
            opng_printf("Recoverable errors found in input. "
                        "Fixing...\n");
            session->flags |= OPNG_NEEDS_NEW_FILE | OPNG_NEEDS_NEW_IDAT;
        }
        else
        {
            opng_error(session->in_fname,
                       "The previous warning(s) indicate recoverable errors",
                       "Rerun this program using -fix");
            return -1;
        }
    }

    /* Check the junk flag. */
    if (session->flags & OPNG_HAS_JUNK)
        session->flags |= OPNG_NEEDS_NEW_FILE;

    /* Check the PNG signature and datastream flags. */
    if (!(session->flags & OPNG_HAS_PNG_SIGNATURE))
        session->flags |= OPNG_NEEDS_NEW_FILE;
    if (session->flags & OPNG_HAS_PNG_DATASTREAM)
    {
        if (options->nz && (session->flags & OPNG_NEEDS_NEW_IDAT))
        {
            opng_error(session->in_fname,
                       "Can't process file without recoding IDAT",
                       "An option that disallows IDAT recoding "
                       "has been enabled");
            return -1;
        }
    }
    else
        session->flags |= OPNG_NEEDS_NEW_IDAT;

    /* Check the digital signature flag. */
    if (session->flags & OPNG_HAS_DIGITAL_SIGNATURE)
    {
        if (options->force)
        {
            opng_printf("Stripping digital signature:\n");
            session->flags |= OPNG_NEEDS_NEW_FILE;
        }
        else
        {
            opng_error(session->in_fname,
                       "The file is digitally signed and can't be processed",
                       "Rerun this program using -force");
            /* FIXME:  "Rerun this program using -strip dSIG"); */
            return -1;
        }
    }

    /* Check the snipped image flag and ensure that snipping from external
     * formats happened at the user's request.
     */
    if ((session->flags & OPNG_HAS_SNIPPED_IMAGES) &&
        !(session->flags & OPNG_IS_PNG_FILE))
    {
        if (!options->snip)
        {
            opng_error(session->in_fname,
                       "The multi-image file could not be converted "
                       "to PNG losslessly",
                       "To allow snipping, rerun this program using -snip");
            return -1;
        }
    }

    /* Init and check the I/O environment before engaging in lengthy trials. */
    if (!options->no_create)
    {
        const char *png_extname =
            ((session->flags & OPNG_IS_PNG_FILE) || options->out) ?
               NULL : "png";
        if (opng_ioenv_init(ioenv, png_extname) == -1)
            return -1;  /* an error message has already been issued */
    }

    /* Find the best parameters and see if it's worth recompressing. */
    if (!options->nz || (session->flags & OPNG_NEEDS_NEW_IDAT))
    {
        opng_init_iterations(session);
        opng_iterate(session);
        opng_finish_iterations(session);
    }
    if (session->flags & OPNG_NEEDS_NEW_IDAT)
        session->flags |= OPNG_NEEDS_NEW_FILE;

    /* Stop here? */
    if (!(session->flags & OPNG_NEEDS_NEW_FILE) &&
        opng_ioenv_get_overwrite_infile(ioenv))
    {
        opng_printf("No output: no update needed.\n");
        return 0;
    }
    if (options->no_create)
    {
        opng_printf("No output: simulation mode.\n");
        return 0;
    }

    /* Write the output file. */
    if (opng_ioenv_begin_output(ioenv) == -1)
        return -1;  /* an error message has already been issued */
    session->out_fname = opng_ioenv_get_out_fname(ioenv);
    if ((session->flags & OPNG_IS_PNG_FILE) &&
         opng_ioenv_get_overwrite_infile(ioenv))
        opng_printf("Output:\n");  /* no need to show the file name here */
    else
        opng_printf("Output: %s\n", session->out_fname);
    if (options->use_stdout)
    {
        /* Don't set stdout mode to binary here.
         * Some systems allow this operation to be done only once,
         * so it's better to do it outside.
         */
        out_stream = stdout;
    }
    else
    {
        out_stream = fopen(session->out_fname, "wb");
        if (out_stream == NULL)
        {
            opng_error(session->out_fname,
                       "Can't open file for writing", NULL);
            return -1;
        }
    }
    for ( ; ; )
    {
        if (session->flags & OPNG_NEEDS_NEW_IDAT)
        {
            /* Write a brand new PNG datastream to the output. */
            result = opng_write_file(session, &session->best_params, out_stream);
            break;
        }
        else
        {
            /* Copy the input PNG datastream to the output. */
            const char *reopen_fname =
                opng_ioenv_get_overwrite_infile(ioenv) ?
                   opng_ioenv_get_bak_fname(ioenv) :
                   opng_ioenv_get_in_fname(ioenv);
            in_stream = fopen(reopen_fname, "rb");
            if (in_stream == NULL)
            {
                opng_error(reopen_fname, "Can't reopen file", NULL);
                result = -1;
                break;
            }
            if (fseek(in_stream, session->in_stats.datastream_offset,
                      SEEK_SET) != 0)
            {
                opng_error(reopen_fname, "Can't reposition file", NULL);
                fclose(in_stream);
                result = -1;
                break;
            }
            session->best_idat_size = session->in_stats.idat_size;
            result = opng_copy_file(session, in_stream, out_stream);
            fclose(in_stream);
            break;
        }
    }
    if (out_stream == stdout)
        fflush(out_stream);
    else if (out_stream != NULL)
        fclose(out_stream);
    if (result < 0)
    {
        opng_ioenv_unroll_output(ioenv);
        return -1;
    }
    if (opng_ioenv_end_output(ioenv) < 0)
        return -1;

    /* Display the output IDAT/file sizes. */
    opng_print_idat_size_difference_line(session->in_stats.idat_size,
                                         session->out_stats.idat_size);
    opng_print_file_size_difference_line(session->in_stats.file_size,
                                         session->out_stats.file_size);
    return 0;
}

/*
 * Creates an optimizer object.
 * This is designed to be thread-safe, but it currently is THREAD-UNSAFE.
 */
opng_optimizer_t *
opng_create_optimizer(void)
{
    opng_optimizer_t *result;

    result = (opng_optimizer_t *)calloc(1, sizeof(struct opng_optimizer));
    return result;
}

/*
 * Adjusts an iteration option.
 */
static int
opng_set_iteration_option(optk_bits_t *result_set,
                          optk_bits_t option_set,
                          optk_bits_t set_mask,
                          const char *preset)
{
    size_t end_idx;

    /* Constrain the result to the valid range. */
    *result_set = option_set & set_mask;
    if (*result_set == 0)
    {
        /* The result set is 0 because the input set is either
         * uninitialized or out of range.
         * Use the default preset only in the former case.
         */
        if (option_set != 0)
            return -1;
        *result_set = optk_rangeset_string_to_bits(preset, &end_idx);
        OPNG_ASSERT(preset[end_idx] == 0, "Invalid iteration preset");
    }
    return 0;
}

/*
 * Sets the user options in an optimizer object.
 */
int
opng_set_options(opng_optimizer_t *optimizer,
                 const struct opng_options *user_options)
{
    struct opng_options *options;
    int preset_index;

    optimizer->options = *user_options;
    options = &optimizer->options;

    if (options->optim_level == 0)
        options->optim_level = OPNG_OPTIM_LEVEL_DEFAULT;
    else if (options->optim_level == OPNG_OPTIM_LEVEL_FASTEST)
        options->nz = 1;
    else if (options->optim_level > OPNG_OPTIM_LEVEL_MAX)
        options->optim_level = OPNG_OPTIM_LEVEL_MAX;
    else if (options->optim_level < OPNG_OPTIM_LEVEL_MIN)
        return -1;

    preset_index = options->optim_level - OPNG_OPTIM_LEVEL_MIN;
    if (opng_set_iteration_option(&options->filter_set,
                                  user_options->filter_set,
                                  OPNG_FILTER_SET_MASK,
                                  filter_presets[preset_index]) < 0)
        return -1;
    if (opng_set_iteration_option(&options->zcompr_level_set,
                                  user_options->zcompr_level_set,
                                  OPNG_ZCOMPR_LEVEL_SET_MASK,
                                  zcompr_level_presets[preset_index]) < 0)
        return -1;
    if (opng_set_iteration_option(&options->zmem_level_set,
                                  user_options->zmem_level_set,
                                  OPNG_ZMEM_LEVEL_SET_MASK,
                                  zmem_level_presets[preset_index]) < 0)
        return -1;
    if (opng_set_iteration_option(&options->zstrategy_set,
                                  user_options->zstrategy_set,
                                  OPNG_ZSTRATEGY_SET_MASK,
                                  zstrategy_presets[preset_index]) < 0)
        return -1;

    /* TODO: Validate the remaining options. */
    return 0;
}

/*
 * Sets the transformer in an optimizer object.
 */
void
opng_set_transformer(opng_optimizer_t *optimizer,
                     opng_transformer_t *transformer)
{
    int check;
    size_t err_objname_offset, err_objname_length;
    const char *err_msg;

    if (optimizer->options.snip)
    {
        check = opng_transform_reset_objects(transformer, "animation",
                    &err_objname_offset, &err_objname_length, &err_msg);
        OPNG_ASSERT(check == 0, "Can't reset animation");
    }
    optimizer->transformer = opng_seal_transformer(transformer);
}

/*
 * Optimizes an image file.
 */
int
opng_optimize_file(opng_optimizer_t *optimizer,
                   const char *in_fname,
                   const char *out_fname,
                   const char *out_dirname)
{
    struct opng_session session;
    const struct opng_options *options;
    opng_ioenv_t *ioenv;
    unsigned int ioenv_flags;
    int result;

    options = &optimizer->options;
    ioenv_flags = 0;
    if (options->backup)
        ioenv_flags |= OPNG_IOENV_BACKUP;
    if (!options->no_clobber)
        ioenv_flags |= OPNG_IOENV_OVERWRITE;
    if (options->preserve)
        ioenv_flags |= OPNG_IOENV_PRESERVE;
    if (options->use_stdin)
    {
        ioenv_flags |= OPNG_IOENV_USE_STDIN;
        optk_fsetmode(stdin, OPTK_FMODE_BINARY);
    }
    if (options->use_stdout)
    {
        ioenv_flags |= OPNG_IOENV_USE_STDOUT;
        optk_fsetmode(stdout, OPTK_FMODE_BINARY);
    }
    ioenv = opng_ioenv_create(in_fname, out_fname, out_dirname, ioenv_flags);

    memset(&session, 0, sizeof(session));
    session.options = options;
    session.transformer = optimizer->transformer;
    opng_init_image(&session.image);
    result = opng_optimize_impl(&session, ioenv);

    ++optimizer->file_count;
    if (result == 0)
    {
        if (session.flags & OPNG_HAS_ERRORS)
        {
            ++optimizer->err_count;
            ++optimizer->fix_count;
        }
    }
    else
        ++optimizer->err_count;

    opng_clear_image(&session.image);
    opng_ioenv_destroy(ioenv);
    opng_printf("\n");
    return result;
}

/*
 * Destroys an optimizer object.
 */
void
opng_destroy_optimizer(opng_optimizer_t *optimizer)
{
    if (optimizer->options.verbose ||
        optimizer->err_count > 0)
        opng_print_status_report(optimizer);
    free(optimizer);
}
