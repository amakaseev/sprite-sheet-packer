/*
 * opngcore/image.h
 * Image manipulation.
 *
 * Copyright (C) 2001-2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef OPNGCORE_IMAGE_H
#define OPNGCORE_IMAGE_H

#ifndef OPNGLIB_INTERNAL
#error This header, internal to opnglib, is not meant to be visible outside.
#endif

#include "png.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * The image structure.
 */
struct opng_image
{
    png_uint_32 width;             /* IHDR */
    png_uint_32 height;
    int bit_depth;
    int color_type;
    int compression_type;
    int filter_type;
    int interlace_type;
    png_bytepp row_pointers;       /* IDAT */
    png_colorp palette;            /* PLTE */
    int num_palette;
    png_bytep trans_alpha;         /* tRNS */
    int num_trans;
    png_color_16p trans_color_ptr;
    png_color_16 trans_color;
    png_color_16p background_ptr;  /* bKGD */
    png_color_16 background;
    png_uint_16p hist;             /* hIST */
    png_color_8p sig_bit_ptr;      /* sBIT */
    png_color_8 sig_bit;
    png_unknown_chunkp unknowns;   /* everything else */
    int num_unknowns;
};


/*
 * Initializes an image object.
 */
void
opng_init_image(struct opng_image *image);

/*
 * Loads an image object from libpng structures.
 */
void
opng_load_image(struct opng_image *image,
                png_structp png_ptr,
                png_infop info_ptr,
                int load_metadata);

/*
 * Stores an image object into libpng structures.
 */
void
opng_store_image(struct opng_image *image,
                 png_structp png_ptr,
                 png_infop info_ptr,
                 int store_metadata);

/*
 * Clears an image object.
 */
void
opng_clear_image(struct opng_image *image);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPNGCORE_UTIL_H */
