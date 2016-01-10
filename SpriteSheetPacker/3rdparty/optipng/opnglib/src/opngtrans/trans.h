/*
 * opngtrans/trans.h
 * Image transformations.
 *
 * Copyright (C) 2011-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef OPNGTRANS_TRANS_H
#define OPNGTRANS_TRANS_H

#ifndef OPNGLIB_INTERNAL
#error This header, internal to opnglib, is not meant to be visible outside.
#endif

#include "opngtrans.h"

#include <stdlib.h>

#include "png.h"

#include "chunksig.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Object IDs.
 *
 * OPNG_ID_FOO represents "foo", and OPNG_ID_FOO_BAR represents "foo.bar",
 * unless otherwise noted. See object_id_map[] in parser.c.
 *
 * The underlying bitset representation is highly efficient, although
 * it restricts the number of recognized IDs. This restriction is
 * acceptable at this time, because only a few IDs are currently in use.
 */
typedef enum
{
    OPNG_ID__NONE           = 0x0000,  /* no object */
    OPNG_ID__UNKNOWN        = 0x0001,  /* unknown object */
    OPNG_ID__RESERVED       = 0x0002,  /* reserved object */

    OPNG_ID_CHUNK_IMAGE     = 0x0010,  /* critical chunk or tRNS */
    OPNG_ID_CHUNK_ANIMATION = 0x0020,  /* APNG chunk: acTL, fcTL or fdAT */
    OPNG_ID_CHUNK_META      = 0x0040,  /* any other ancillary chunk */

    OPNG_ID_ALL                   = 0x00000100,  /* "all" */
    OPNG_ID_IMAGE_ALPHA           = 0x00000200,  /* "image.alpha" */
    OPNG_ID_IMAGE_CHROMA_BT601    = 0x00000400,  /* "image.chroma.bt601" */
    OPNG_ID_IMAGE_CHROMA_BT709    = 0x00000800,  /* "image.chroma" or "image.chroma.bt709" */
    OPNG_ID_ANIMATION             = 0x00001000,  /* "animation" */

    OPNG_ID_IMAGE_PRECISION       = 0x00010000,  /* "image.precision" */
    OPNG_ID_IMAGE_ALPHA_PRECISION = 0x00020000,  /* "image.alpha.precision" */
    OPNG_ID_IMAGE_GRAY_PRECISION  = 0x00040000,  /* "image.gray.precision" */
    OPNG_ID_IMAGE_RGB_PRECISION   = 0x00080000,  /* "image.rgb.precision" */
    OPNG_ID_IMAGE_RED_PRECISION   = 0x00100000,  /* "image.red.precision" */
    OPNG_ID_IMAGE_GREEN_PRECISION = 0x00200000,  /* "image.green.precision" */
    OPNG_ID_IMAGE_BLUE_PRECISION  = 0x00400000   /* "image.blue.precision" */
} opng_id_t;

/*
 * Object ID sets: "all chunks", "can reset", "can set", "can strip", etc.
 */
enum
{
    OPNG_IDSET_CHUNK =
        OPNG_ID_CHUNK_IMAGE |
        OPNG_ID_CHUNK_ANIMATION |
        OPNG_ID_CHUNK_META,
    OPNG_IDSET_CAN_RESET =
        OPNG_ID_IMAGE_ALPHA |
        OPNG_ID_IMAGE_CHROMA_BT601 |
        OPNG_ID_IMAGE_CHROMA_BT709 |
        OPNG_ID_ANIMATION,
    OPNG_IDSET_CAN_SET =
        OPNG_ID_IMAGE_PRECISION |
        OPNG_ID_IMAGE_ALPHA_PRECISION |
        OPNG_ID_IMAGE_GRAY_PRECISION |
        OPNG_ID_IMAGE_RGB_PRECISION |
        OPNG_ID_IMAGE_RED_PRECISION |
        OPNG_ID_IMAGE_GREEN_PRECISION |
        OPNG_ID_IMAGE_BLUE_PRECISION,
    OPNG_IDSET_CAN_STRIP =
        OPNG_ID_ALL |
        OPNG_ID_CHUNK_META,
    OPNG_IDSET_CAN_PROTECT =
        OPNG_IDSET_CAN_STRIP
};

/*
 * The transformer structure.
 */
struct opng_transformer
{
    struct opng_sigs strip_sigs;
    struct opng_sigs protect_sigs;
    opng_id_t strip_ids;
    opng_id_t protect_ids;
    opng_id_t reset_ids;
    int alpha_precision;
    int red_precision;
    int green_precision;
    int blue_precision;
};

/*
 * Returns 1 if the given chunk ought to be stripped, or 0 otherwise.
 */
int
opng_transform_query_strip_chunk(const opng_transformer_t *transformer,
                                 png_byte *chunk_sig);

/*
 * Retrieves the precision values to be set for each channel.
 */
void
opng_transform_query_set_precision(const opng_transformer_t *transformer,
                                   int *alpha_precision_ptr,
                                   int *red_precision_ptr,
                                   int *green_precision_ptr,
                                   int *blue_precision_ptr);

/*
 * Applies all set/reset transformations to the given libpng image structures.
 * Returns 1 if at least one transformation has been applied, or 0 otherwise.
 */
int
opng_transform_apply(const opng_transformer_t *transformer,
                     png_structp libpng_ptr, png_infop info_ptr);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPNGTRANS_TRANS_H */
