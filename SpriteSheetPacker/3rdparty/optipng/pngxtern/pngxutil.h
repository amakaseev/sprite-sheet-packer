/*
 * pngxutil.h - libpng extension utilities.
 *
 * Copyright (C) 2003-2011 Cosmin Truta.
 * This software is distributed under the same licensing and warranty terms
 * as libpng.
 */

#ifndef PNGXUTIL_H
#define PNGXUTIL_H

#include "png.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Store data into the info structure. */
void PNGAPI pngx_set_compression_type
   (png_structp png_ptr, png_infop info_ptr, int compression_type);
void PNGAPI pngx_set_filter_type
   (png_structp png_ptr, png_infop info_ptr, int filter_type);
void PNGAPI pngx_set_interlace_type
   (png_structp png_ptr, png_infop info_ptr, int interlace_type);


#if PNG_LIBPNG_VER >= 10400
typedef png_alloc_size_t pngx_alloc_size_t;
#else
/* Compatibility backport of png_alloc_size_t */
typedef png_uint_32 pngx_alloc_size_t;
#endif

#ifdef PNG_INFO_IMAGE_SUPPORTED
/* Allocate memory for the row pointers.
 * Use filler to initialize the rows if it is non-negative.
 * On success return the newly-allocated row pointers.
 * On failure issue a png_error() or return NULL,
 * depending on the status of PNG_FLAG_MALLOC_NULL_MEM_OK.
 */
png_bytepp PNGAPI pngx_malloc_rows
   (png_structp png_ptr, png_infop info_ptr, int filler);
png_bytepp PNGAPI pngx_malloc_rows_extended
   (png_structp png_ptr, png_infop info_ptr,
    pngx_alloc_size_t min_row_size, int filler);
#endif


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif /* PNGXUTIL_H */
