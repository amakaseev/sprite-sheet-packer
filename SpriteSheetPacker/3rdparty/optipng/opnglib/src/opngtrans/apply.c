/*
 * opngtrans/apply.c
 * Apply the image transformations.
 *
 * Copyright (C) 2011-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include "opngtrans.h"

#include "png.h"

#define OPNGLIB_INTERNAL
#include "trans.h"
#include "../opngcore/util.h"


/*
 * The chromaticity specifications.
 */
typedef enum
{
    chroma_spec_bt601 = 0,
    chroma_spec_bt709 = 1
} chroma_spec_t;

/*
 * The ITU BT.601 chromaticity coefficients.
 * In floating-point arithmetic:
 *    Y = 0.299 * R + 0.587 * G + 0.114 * B
 * In 16-bit fixed-point arithmetic:
 *    Y = 19595 * R + 38470 * G + 7471 * B
 */
enum
{
    Kr_bt601 = 19595U,
    Kg_bt601 = 38470U,
    Kb_bt601 =  7471U
};

/*
 * The ITU BT.709 chromaticity coefficients.
 * In floating-point arithmetic:
 *    Y = 0.2126 * R + 0.7152 * G + 0.0722 * B
 * In 16-bit fixed-point arithmetic:
 *    Y = 13932.9536 * R + 46871.3472 * G + 4731.6992 * B
 */
enum
{
    Kr_bt709 = 13933U,
    Kg_bt709 = 46871U,
    Kb_bt709 =  4731U
};

/*
 * Converts an 8-bit RGB pixel to grayscale, as specified in ITU BT.601.
 */
static void
opng_reset_chroma8_bt601(png_bytep rgb8)
{
    /* Use 16-bit multiplications and 24-bit additions. */
    png_uint_32 r24 = rgb8[0] * (png_uint_32)Kr_bt601;
    png_uint_32 g24 = rgb8[1] * (png_uint_32)Kg_bt601;
    png_uint_32 b24 = rgb8[2] * (png_uint_32)Kb_bt601;
    png_uint_32 y24 = r24 + g24 + b24;
    png_byte y8 = (png_byte)((y24 + 32767U) / 65535U);
    rgb8[0] = y8;
    rgb8[1] = y8;
    rgb8[2] = y8;
}

/*
 * Converts an 8-bit RGB pixel to grayscale, as specified in ITU BT.709.
 */
static void
opng_reset_chroma8_bt709(png_bytep rgb8)
{
    /* Use 16-bit multiplications and 24-bit additions. */
    png_uint_32 r24 = rgb8[0] * (png_uint_32)Kr_bt709;
    png_uint_32 g24 = rgb8[1] * (png_uint_32)Kg_bt709;
    png_uint_32 b24 = rgb8[2] * (png_uint_32)Kb_bt709;
    png_uint_32 y24 = r24 + g24 + b24;
    png_byte y8 = (png_byte)((y24 + 32767U) / 65535U);
    rgb8[0] = y8;
    rgb8[1] = y8;
    rgb8[2] = y8;
}

/*
 * Converts a 16-bit RGB pixel to grayscale, as specified in ITU BT.601.
 */
static void
opng_reset_chroma16_bt601(png_bytep rgb16)
{
    /* Use 16-bit multiplications and 32-bit additions. */
    unsigned int r16 = png_get_uint_16(rgb16 + 0);
    unsigned int g16 = png_get_uint_16(rgb16 + 2);
    unsigned int b16 = png_get_uint_16(rgb16 + 4);
    png_uint_32 r32 = r16 * (png_uint_32)Kr_bt601;
    png_uint_32 g32 = g16 * (png_uint_32)Kg_bt601;
    png_uint_32 b32 = b16 * (png_uint_32)Kb_bt601;
    png_uint_32 y32 = r32 + g32 + b32;
    unsigned int y16 = (unsigned int)((y32 + 32767U) / 65535U);
    png_save_uint_16(rgb16 + 0, y16);
    png_save_uint_16(rgb16 + 2, y16);
    png_save_uint_16(rgb16 + 4, y16);
}

/*
 * Converts a 16-bit RGB pixel to grayscale, as specified in ITU BT.709.
 */
static void
opng_reset_chroma16_bt709(png_bytep rgb16)
{
    /* Use 16-bit multiplications and 32-bit additions. */
    unsigned int r16 = png_get_uint_16(rgb16 + 0);
    unsigned int g16 = png_get_uint_16(rgb16 + 2);
    unsigned int b16 = png_get_uint_16(rgb16 + 4);
    png_uint_32 r32 = r16 * (png_uint_32)Kr_bt709;
    png_uint_32 g32 = g16 * (png_uint_32)Kg_bt709;
    png_uint_32 b32 = b16 * (png_uint_32)Kb_bt709;
    png_uint_32 y32 = r32 + g32 + b32;
    unsigned int y16 = (unsigned int)((y32 + 32767U) / 65535U);
    png_save_uint_16(rgb16 + 0, y16);
    png_save_uint_16(rgb16 + 2, y16);
    png_save_uint_16(rgb16 + 4, y16);
}

/*
 * The table of chroma reset functions.
 */
static
void (*opng_reset_chroma_fn_table[2][2])(png_bytep) =
{
    { opng_reset_chroma8_bt601, opng_reset_chroma16_bt601 },
    { opng_reset_chroma8_bt709, opng_reset_chroma16_bt709 }
};

/*
 * Sets the precision of a 2-bit sample to a lower value (i.e. 1),
 * then scales the result back to 2 bits.
 */
static void
opng_set_precision2(png_bytep samples_ptr, int new_precision)
{
    unsigned int values = *samples_ptr;
    values &= 0xaa;
    values |= (values >> 1);
    *samples_ptr = (png_byte)values;
    (void)new_precision;
}

/*
 * Sets the precision of a 4-bit sample to a lower value,
 * then scales the result back to 2 bits.
 */
static void
opng_set_precision4(png_bytep samples_ptr, int new_precision)
{
    unsigned int values = *samples_ptr;
    switch (new_precision)
    {
    case 1:
        values &= 0x88;
        values |= (values >> 1);
        values |= (values >> 2);
        break;
    case 2:
        values &= 0xcc;
        values |= (values >> 2);
        break;
    case 3:
        values &= 0xee;
        values |= ((values & 0x88) >> 3);
        break;
    default:
        values = 0;
        /* assert(0); */
    }
    *samples_ptr = (png_byte)values;
}

/*
 * Sets the precision of an 8-bit sample to a lower value,
 * then scales the result back to 8 bits.
 */
static void
opng_set_precision8(png_bytep sample_ptr, int new_precision)
{
    unsigned int old_value = *sample_ptr;
    unsigned int chop_value = old_value >> (8 - new_precision);
    unsigned int chop_max = (1U << new_precision) - 1;
    unsigned int new_value = (chop_value * 255 + chop_max / 2) / chop_max;
    *sample_ptr = (png_byte)new_value;
}

/*
 * Sets the precision of a 16-bit sample to a lower value,
 * then scales the result back to 16 bits.
 */
static void
opng_set_precision16(png_bytep sample_ptr, int new_precision)
{
    unsigned int old_value = png_get_uint_16(sample_ptr);
    unsigned int chop_value = old_value >> (16 - new_precision);
    unsigned int chop_max = (1U << new_precision) - 1;
    unsigned int new_value =
        (unsigned int)
            (((png_uint_32)chop_value * 65535U + chop_max / 2) / chop_max);
    png_save_uint_16(sample_ptr, new_value);
}

/*
 * Sets the precision of the alpha channel to a given value;
 * if this value is 0, resets (i.e. sets to MAX) the alpha channel.
 * Returns 1 if this operation is applied, or 0 otherwise.
 */
static int
opng_transform_set_alpha_precision(png_structp libpng_ptr, png_infop info_ptr,
                                   int alpha_precision)
{
    int result;
    png_uint_32 width, height;
    int bit_depth, color_type;
    png_bytep trans_alpha;
    int num_trans;
    png_bytepp row_ptr;
    png_bytep alpha_ptr;
    unsigned int num_channels, channel_size, pixel_size;
    unsigned int alpha_offset;
    png_uint_32 i, j;
    int k;

    void (*set_precision_fn)(png_bytep, int);

    png_get_IHDR(libpng_ptr, info_ptr,
                 &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
    if (bit_depth <= 8)
    {
        if (alpha_precision >= 8)
            return 0;
        set_precision_fn = opng_set_precision8;
    }
    else
    {
        if (alpha_precision >= 16)
            return 0;
        set_precision_fn = opng_set_precision16;
    }

    result = 0;

    if (png_get_valid(libpng_ptr, info_ptr, PNG_INFO_tRNS))
    {
        if (alpha_precision > 0)
        {
            /* Set the precision of tRNS samples. */
            png_get_tRNS(libpng_ptr, info_ptr, &trans_alpha, &num_trans, NULL);
            if (trans_alpha != NULL)
            {
                for (k = 0; k < num_trans; ++k)
                    opng_set_precision8(trans_alpha + k, alpha_precision);
                result = 1;
            }
            /* trans_color needs no transformation */
        }
        else  /* alpha_precision == 0 */
        {
            /* Invalidate tRNS. */
            png_set_invalid(libpng_ptr, info_ptr, PNG_INFO_tRNS);
            result = 1;
        }
        /* Continue regardless whether PLTE is present.
         * PLTE may exist even if the color type is not palette.
         */
    }

    if (color_type & PNG_COLOR_MASK_ALPHA)
    {
        row_ptr = png_get_rows(libpng_ptr, info_ptr);
        channel_size = (bit_depth > 8) ? 2 : 1;
        num_channels = png_get_channels(libpng_ptr, info_ptr);
        pixel_size = num_channels * channel_size;
        alpha_offset = pixel_size - channel_size;
        if (alpha_precision > 0)
        {
            /* Set the precision of the alpha channel. */
            for (i = 0; i < height; ++i, ++row_ptr)
            {
                alpha_ptr = *row_ptr + alpha_offset;
                for (j = 0; j < width; ++j, alpha_ptr += pixel_size)
                    (*set_precision_fn)(alpha_ptr, alpha_precision);
            }
        }
        else  /* alpha_precision == 0 */
        {
            /* Set the alpha channel to opaque. */
            for (i = 0; i < height; ++i, ++row_ptr)
            {
                alpha_ptr = *row_ptr + alpha_offset;
                for (j = 0; j < width; ++j, alpha_ptr += pixel_size)
                {
                    alpha_ptr[0] = 255;
                    alpha_ptr[channel_size - 1] = 255;
                }
            }
        }
        result = 1;
    }

    return result;
}

/*
 * Sets the precision of the RGB (or gray) channels to a given value.
 * Returns 1 if this operation is applied, or 0 otherwise.
 */
static int
opng_transform_set_rgb_precision(png_structp libpng_ptr, png_infop info_ptr,
                                 int red_precision,
                                 int green_precision,
                                 int blue_precision)
{
    png_uint_32 width, height;
    int bit_depth, color_type;
    int sample_depth;
    png_colorp palette;
    int num_palette;
    png_bytepp row_ptr;
    png_bytep pixel_ptr;
    png_uint_32 row_size;
    unsigned int num_channels, channel_size, pixel_size;
    unsigned int red_offset, green_offset, blue_offset;
    int gray_precision;
    png_uint_32 i, j;
    int k;

    void (*set_precision_fn)(png_bytep, int);

    png_get_IHDR(libpng_ptr, info_ptr,
                 &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
    if (bit_depth <= 8)
    {
        sample_depth = 8;
        set_precision_fn = opng_set_precision8;
    }
    else
    {
        sample_depth = 16;
        set_precision_fn = opng_set_precision16;
    }
    if (red_precision >= sample_depth &&
            green_precision >= sample_depth && blue_precision >= sample_depth)
        return 0;

    /* Set the precision of the color palette. */
    if (png_get_PLTE(libpng_ptr, info_ptr, &palette, &num_palette))
    {
        for (k = 0; k < num_palette; ++k)
        {
            if (red_precision < sample_depth)
                opng_set_precision8(&palette[k].red, red_precision);
            if (green_precision < sample_depth)
                opng_set_precision8(&palette[k].green, green_precision);
            if (blue_precision < sample_depth)
                opng_set_precision8(&palette[k].blue, blue_precision);
        }
        /* Exit early if the color type is palette.
         * PLTE may exist even if the color type is not palette.
         */
        if (color_type & PNG_COLOR_MASK_PALETTE)
            return 1;
    }

    /* Set the precision of pixels. */
    row_ptr = png_get_rows(libpng_ptr, info_ptr);
    channel_size = (bit_depth > 8) ? 2 : 1;
    num_channels = png_get_channels(libpng_ptr, info_ptr);
    pixel_size = num_channels * channel_size;
    if (color_type & PNG_COLOR_MASK_COLOR)
    {
        red_offset = 0;
        green_offset = channel_size;
        blue_offset = 2 * channel_size;
        for (i = 0; i < height; ++i, ++row_ptr)
        {
            pixel_ptr = *row_ptr;
            for (j = 0; j < width; ++j, pixel_ptr += pixel_size)
            {
                if (red_precision < sample_depth)
                    (*set_precision_fn)(pixel_ptr + red_offset, red_precision);
                if (green_precision < sample_depth)
                    (*set_precision_fn)(pixel_ptr + green_offset, green_precision);
                if (blue_precision < sample_depth)
                    (*set_precision_fn)(pixel_ptr + blue_offset, blue_precision);
            }
        }
        return 1;
    }
    else  /* grayscale */
    {
        /* The precision of the gray channel is
         * max(red_precision, green_precision, blue_precision).
         */
        gray_precision = red_precision;
        if (gray_precision < green_precision)
            gray_precision = green_precision;
        if (gray_precision < blue_precision)
            gray_precision = blue_precision;
        if (gray_precision >= bit_depth)
            return 0;
        if (bit_depth == 2)
        {
            row_size = (width / 4) + ((width & 3) ? 1 : 0);
            set_precision_fn = opng_set_precision2;
        }
        else if (bit_depth == 4)
        {
            row_size = (width / 2) + ((width & 1) ? 1 : 0);
            set_precision_fn = opng_set_precision4;
        }
        else  /* bit_depth == 8 || bit_depth == 16 */
            row_size = width;
        for (i = 0; i < height; ++i, ++row_ptr)
        {
            pixel_ptr = *row_ptr;
            for (j = 0; j < row_size; ++j, pixel_ptr += pixel_size)
                (*set_precision_fn)(pixel_ptr, gray_precision);
        }
        return 1;
    }

    return 0;
}

/*
 * Resets (i.e. sets to zero) the chroma channel.
 * Returns 1 if this operation is applied, or 0 otherwise.
 */
static int
opng_transform_reset_chroma(png_structp libpng_ptr, png_infop info_ptr,
                            chroma_spec_t chroma_spec)
{
    png_uint_32 width, height;
    int bit_depth, color_type;
    png_colorp palette;
    int num_palette;
    png_colorp palette_entry_ptr;
    png_bytepp row_ptr;
    png_bytep pixel_ptr;
    unsigned int channel_size, pixel_size;
    png_byte rgb_buf[6];
    png_uint_32 i, j;
    int k;

    void (*reset_chroma_fn)(png_bytep);

    png_get_IHDR(libpng_ptr, info_ptr,
                 &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

    channel_size = (bit_depth <= 8) ? 1 : 2;
    reset_chroma_fn = opng_reset_chroma_fn_table[chroma_spec][channel_size - 1];

    if (color_type & PNG_COLOR_MASK_PALETTE)
    {
        /* Reset chroma in PLTE. */
        if (!png_get_PLTE(libpng_ptr, info_ptr, &palette, &num_palette))
            return 0;
        palette_entry_ptr = palette;
        for (k = 0; k < num_palette; ++k, ++palette_entry_ptr)
        {
            /* This is not very efficient, but the iterations are few. */
            rgb_buf[0] = palette_entry_ptr->red;
            rgb_buf[1] = palette_entry_ptr->green;
            rgb_buf[2] = palette_entry_ptr->blue;
            (*reset_chroma_fn)(rgb_buf);
            palette_entry_ptr->red =
                palette_entry_ptr->green =
                    palette_entry_ptr->blue = rgb_buf[0];
        }
    }
    else if (color_type & PNG_COLOR_MASK_COLOR)
    {
        /* Reset chroma in IDAT. */
        pixel_size = png_get_channels(libpng_ptr, info_ptr) * channel_size;
        row_ptr = png_get_rows(libpng_ptr, info_ptr);
        for (i = 0; i < height; ++i, ++row_ptr)
        {
            pixel_ptr = *row_ptr;
            for (j = 0; j < width; ++j, pixel_ptr += pixel_size)
                (*reset_chroma_fn)(pixel_ptr);
        }
    }
    else
    {
        /* The pixels are grayscale; do nothing. */
        return 0;
    }

    return 1;
}

/*
 * Invalidate the unsafe-to-copy metadata that cannot reliably remain
 * after transformations.
 */
static void
opng_transform_invalidate_meta(png_structp libpng_ptr, png_infop info_ptr)
{
    int color_type;

    color_type = png_get_color_type(libpng_ptr, info_ptr);
    if (!(color_type & PNG_COLOR_MASK_PALETTE))
    {
        /* PLTE, if it exists in this situation, has the same role as sPLT. */
        png_set_invalid(libpng_ptr, info_ptr, PNG_INFO_PLTE);
    }
#ifdef PNG_sPLT_SUPPORTED
    png_set_invalid(libpng_ptr, info_ptr, PNG_INFO_sPLT);
#endif
#ifdef PNG_hIST_SUPPORTED
    png_set_invalid(libpng_ptr, info_ptr, PNG_INFO_hIST);
#endif
#ifdef PNG_sBIT_SUPPORTED
    png_set_invalid(libpng_ptr, info_ptr, PNG_INFO_hIST);
#endif
    /* FIXME: Invalidate the unknown unsafe-to-copy chunks. */
}

/*
 * Applies all set/reset transformations to the given libpng image structures.
 */
int
opng_transform_apply(const opng_transformer_t *transformer,
                     png_structp libpng_ptr, png_infop info_ptr)
{
    int result;
    int alpha_precision;
    int red_precision;
    int green_precision;
    int blue_precision;

    result = 0;

    /* Reset the chroma channel. */
    /* Apply this operation before setting the sample precision. */
    if (transformer->reset_ids & OPNG_ID_IMAGE_CHROMA_BT601)
    {
        if (opng_transform_reset_chroma(libpng_ptr, info_ptr,
                                        chroma_spec_bt601))
            result = 1;
    }
    else if (transformer->reset_ids & OPNG_ID_IMAGE_CHROMA_BT709)
    {
        if (opng_transform_reset_chroma(libpng_ptr, info_ptr,
                                        chroma_spec_bt709))
            result = 1;
    }

    /* Query the precision of samples. */
    opng_transform_query_set_precision(transformer,
                                       &alpha_precision,
                                       &red_precision,
                                       &green_precision,
                                       &blue_precision);

    /* Modify or reset the alpha samples. */
    if (transformer->reset_ids & OPNG_ID_IMAGE_ALPHA)
    {
        if (opng_transform_set_alpha_precision(libpng_ptr, info_ptr, 0))
            result = 1;
    }
    else
    {
        if (opng_transform_set_alpha_precision(libpng_ptr, info_ptr,
                                               alpha_precision))
            result = 1;
    }

    /* Modify the RGB or grayscale samples. */
    if (opng_transform_set_rgb_precision(libpng_ptr, info_ptr,
                                         red_precision,
                                         green_precision,
                                         blue_precision))
        result = 1;

    if (result)
        opng_transform_invalidate_meta(libpng_ptr, info_ptr);
    return result;
}
