/*
 * pngxtern.h - external file format processing for libpng.
 *
 * Copyright (C) 2003-2011 Cosmin Truta.
 * This software is distributed under the same licensing and warranty terms
 * as libpng.
 */

#ifndef PNGXTERN_H
#define PNGXTERN_H

#include "png.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * A user-defined function that retrieves a random-access
 * (i.e. fseek-able) FILE* stream from io_ptr.
 */
typedef FILE *(*pngx_get_FILE_ptr)(png_voidp io_ptr);


/*
 * Read the contents of an image file and store it into the given
 * libpng structures.
 *
 * The currently recognized file formats are:
 * PNG (standalone), PNG (datastream), BMP, GIF, PNM and TIFF.
 *
 * The function reads either the first or the most relevant image,
 * depending on the format.  For example, embedded thumbnails, if
 * present, are skipped.
 *
 * Reading is performed upon a random-access FILE* stream, obtained
 * by applying the given get_FILE_fn function to libpng's io_ptr.
 *
 * On success, the function returns the number of images contained
 * by the image file, which can be greater than 1 for formats like
 * GIF or TIFF.  If the function finds more than one image but does
 * not perform a complete image count, it returns an upper bound.
 * The function stores the short and/or the long format name
 * (e.g. "PPM", "Portable Pixmap") into the given name pointers,
 * if they are non-null.
 *
 * If the function fails to detect a known format, it rewinds the
 * FILE* stream stored in io_ptr and returns 0.
 * On other errors (e.g. read error or decoding error), the function
 * issues a png_error().
 */
int PNGAPI pngx_read_image(png_structp png_ptr, png_infop info_ptr,
                           pngx_get_FILE_ptr get_FILE_fn,
                           png_const_charpp fmt_name_ptr,
                           png_const_charpp fmt_long_name_ptr);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* PNGXTERN_H */
