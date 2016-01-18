/*
 * opngcore/codec.h
 * PNG encoding and decoding.
 *
 * Copyright (C) 2001-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef OPNGCORE_CODEC_H
#define OPNGCORE_CODEC_H

#ifndef OPNGLIB_INTERNAL
#error This header, internal to opnglib, is not meant to be visible outside.
#endif

#include "opngtrans.h"
#include "integer.h"
#include "io.h"

#include <limits.h>
#include <stdio.h>

#include "opngreduc.h"
#include "png.h"

#include "image.h"
#include "../opngtrans/trans.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * The encoding parameters structure.
 */
struct opng_encoding_params
{
    int filter;
    int interlace;
    int zcompr_level;
    int zmem_level;
    int zstrategy;
    int zwindow_bits;
};

/*
 * The encoding statistics structure.
 */
struct opng_encoding_stats
{
    png_uint_32 flags;
    png_uint_32 plte_trns_size;
    optk_fsize_t idat_size;
    optk_fsize_t file_size;
    optk_foffset_t datastream_offset;
};

/*
 * The codec context structure.
 * Everything that libpng and its callbacks use is found in here.
 * Although this structure is exposed so that it can be placed on
 * the stack, the caller must pretend not to know what's inside.
 */
struct opng_codec_context
{
    struct opng_image *image;
    struct opng_encoding_stats *stats;
    FILE *stream;
    const char *fname;
    png_structp libpng_ptr;
    png_infop info_ptr;
    const opng_transformer_t *transformer;
    optk_foffset_t crt_idat_offset;
    optk_fsize_t crt_idat_size;
    optk_fsize_t expected_idat_size;
    png_uint_32 crt_idat_crc;
    int crt_chunk_is_allowed;
    int crt_chunk_is_idat;
};

/*
 * Encoding flags.
 */
enum
{
    OPNG_IS_PNG_FILE           = 0x0001,
    OPNG_HAS_PNG_DATASTREAM    = 0x0002,
    OPNG_HAS_PNG_SIGNATURE     = 0x0004,
    OPNG_HAS_DIGITAL_SIGNATURE = 0x0008,
    OPNG_HAS_MULTIPLE_IMAGES   = 0x0010,
    OPNG_HAS_SNIPPED_IMAGES    = 0x0020,
    OPNG_HAS_STRIPPED_METADATA = 0x0040,
    OPNG_HAS_JUNK              = 0x0080,
    OPNG_HAS_ERRORS            = 0x0100,
    OPNG_NEEDS_NEW_FILE        = 0x1000,
    OPNG_NEEDS_NEW_IDAT        = 0x2000
};

/*
 * Chunk-level IDAT constants.
 * The max IDAT chunk size is as in the PNG spec.
 * IDAT chunks up to the "large size" need not be split.
 * IDAT chunks larger than the "max size" must be split.
 */
enum
{
    OPNG_IDAT_CHUNK_SIZE_LARGE = (png_uint_32)0x40000000UL,  /* soft limit */
    OPNG_IDAT_CHUNK_SIZE_MAX   = (png_uint_32)0x7fffffffUL   /* hard limit */
};

/*
 * File-level IDAT constants.
 * The (global) IDAT size is the sum of all IDAT chunk sizes.
 */
enum
{
    /* This is a gigantic and yet practical soft limit. */
    OPNG_IDAT_SIZE_MAX = (optk_fsize_t)OPTK_INT64_MAX
};

/*
 * Initializes a codec context object.
 */
void
opng_init_codec_context(struct opng_codec_context *context,
                        struct opng_image *image,
                        struct opng_encoding_stats *stats,
                        optk_fsize_t expected_idat_size,
                        const opng_transformer_t *transformer);

/*
 * Decodes an image from an image file stream.
 * The image may be either in PNG format or in an external file format.
 * The function returns 0 on success or -1 on error.
 */
int
opng_decode_image(struct opng_codec_context *context,
                  FILE *stream,
                  const char *fname,
                  const char **format_name_ptr,
                  const char **format_xdesc_ptr);

/*
 * Attempts to reduce the imported image.
 * The function returns a mask of successful reductions (0 for no reductions),
 * or -1 on error.
 * No error is normally expected to occur; if it does, it indicates a defect.
 */
int
opng_decode_reduce_image(struct opng_codec_context *context,
                         int reductions);

/*
 * Attempts to set and/or reset image data objects within the imported image.
 * The function returns 1 if at least one transformation has been applied,
 * or 0 otherwise.
 */
int
opng_decode_transform_image(struct opng_codec_context *context);

/*
 * Stops the decoder.
 * Frees the stored PNG image data and clears the internal image object,
 * if required.
 */
void
opng_decode_finish(struct opng_codec_context *context,
                   int free_data);

/*
 * Encodes an image to a PNG file stream.
 * If the output file stream is NULL, PNG encoding is still done,
 * and statistics are still collected, but no actual data is written.
 * The function returns 0 on success or -1 on error.
 */
int
opng_encode_image(struct opng_codec_context *context,
                  const struct opng_encoding_params *params,
                  FILE *stream,
                  const char *fname);

/*
 * Stops the decoder.
 * Frees the stored PNG image data and clears the internal image object,
 * if required.
 */
void
opng_encode_finish(struct opng_codec_context *context);

/*
 * Copies a PNG file stream to another PNG file stream.
 * The function returns 0 on success or -1 on error.
 */
int
opng_copy_png(struct opng_codec_context *context,
              FILE *in_stream, const char *in_fname,
              FILE *out_stream, const char *out_fname);


/*
 * The chunk signatures recognized and handled by this codec.
 */
extern const png_byte opng_sig_PLTE[4];
extern const png_byte opng_sig_tRNS[4];
extern const png_byte opng_sig_IDAT[4];
extern const png_byte opng_sig_IEND[4];
extern const png_byte opng_sig_bKGD[4];
extern const png_byte opng_sig_hIST[4];
extern const png_byte opng_sig_sBIT[4];
extern const png_byte opng_sig_dSIG[4];
extern const png_byte opng_sig_acTL[4];
extern const png_byte opng_sig_fcTL[4];
extern const png_byte opng_sig_fdAT[4];

/*
 * Tests whether the given chunk is an image chunk.
 * An image chunk is a chunk that stores image data: a critical chunk or tRNS.
 *
 * It can be argued that APNG fdAT is also an image chunk, but we don't
 * consider that here. We handle APNG separately.
 */
int
opng_is_image_chunk(const png_byte *chunk_type);

/*
 * Tests whether the given chunk is a metadata chunk.
 * Ancillary chunks other than tRNS are all considered to store metadata,
 * but see the comment about APNG above.
 */
int
opng_is_metadata_chunk(const png_byte *chunk_type);

/*
 * Tests whether the given chunk is an APNG chunk.
 */
int
opng_is_apng_chunk(const png_byte *chunk_type);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPNGCORE_CODEC_H */
