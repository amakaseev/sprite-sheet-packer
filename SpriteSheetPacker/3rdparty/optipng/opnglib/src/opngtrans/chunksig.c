/*
 * opngtrans/chunksig.c
 * Chunk signatures.
 *
 * Copyright (C) 2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#if 'A' != 0x41 || 'Z' != 0x5a || 'a' != 0x61 || 'z' != 0x7a
#error This module requires ASCII compatibility.
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define OPNGLIB_INTERNAL
#include "chunksig.h"


/*
 * Initializes a signature set.
 */
void
opng_sigs_init(struct opng_sigs *sigs)
{
    memset(sigs, 0, sizeof(struct opng_sigs));
    sigs->sorted = 1;
}

/*
 * Clears a signature set.
 */
void
opng_sigs_clear(struct opng_sigs *sigs)
{
    free(sigs->buffer);
    opng_sigs_init(sigs);
}

/*
 * Compares two chunk signatures, for the benefit of qsort.
 */
static int
opng_sigs_cmp(const void *sig1, const void *sig2)
{
    return memcmp(sig1, sig2, 4);
}

/*
 * Sorts and uniq's the array buffer that stores chunk signatures.
 */
void
opng_sigs_sort_uniq(struct opng_sigs *sigs)
{
    png_byte *buffer;
    size_t count;
    size_t i, j;

    if (sigs->sorted)
        return;

    buffer = sigs->buffer;
    count = sigs->count;

    /* Sort. */
    qsort(buffer, count, 4, opng_sigs_cmp);

    /* Uniq. */
    for (i = 1, j = 0; i < count; ++i)
    {
        if (memcmp(buffer + i * 4, buffer + j * 4, 4) == 0)
            continue;
        if (++j == i)
            continue;
        memcpy(buffer + j * 4, buffer + i * 4, 4);
    }
    sigs->count = ++j;

    sigs->sorted = 1;
}

/*
 * Returns 1 if a chunk signature is found in the given set,
 * or 0 otherwise.
 */
int
opng_sigs_find(const struct opng_sigs *sigs, const png_byte *chunk_sig)
{
    int left, right, mid, cmp;

    assert(sigs->sorted);

    /* Perform a binary search. */
    left = 0;
    right = sigs->count;
    while (left < right)
    {
        mid = (left + right) / 2;
        cmp = memcmp(chunk_sig, sigs->buffer + mid * 4, 4);
        if (cmp == 0)
            return 1;  /* found */
        else if (cmp < 0)
            right = mid - 1;
        else  /* cmp > 0 */
            left = mid + 1;
    }
    return 0;  /* not found */
}

/*
 * Converts a chunk name to a chunk signature and adds it to
 * a chunk signature set.
 */
int
opng_sigs_add(struct opng_sigs *sigs, const char *chunk_name)
{
    const png_byte *chunk_sig;
    void *new_buffer;

    chunk_sig = (const png_byte *)chunk_name;

    /* If the maximum buffer size is reached, sort the buffer, remove
     * duplicates and check if the signature to be added already exists.
     * This allows the opng_sigs structure to maintain the properties
     * of a set.
     *
     * This strategy allows fast insertions without interleaved lookups,
     * followed by fast lookups without other insertions.
     */
    if (sigs->count >= OPNG_SIGS_SIZE_MAX)
    {
        opng_sigs_sort_uniq(sigs);
        if (opng_sigs_find(sigs, chunk_sig))
            return 0;
    }

    /* If the current buffer is filled, reallocate a larger size. */
    if (sigs->count >= sigs->capacity)
    {
        if (sigs->capacity == 0)
            sigs->capacity = 32;
        else
            sigs->capacity *= 2;
        new_buffer = realloc(sigs->buffer, sigs->capacity * 4);
        if (new_buffer == NULL)
            return -1;
        sigs->buffer = (png_byte *)new_buffer;
    }

    /* Append the new signature to the buffer, which is no longer
     * considered to be sorted.
     */
    memcpy(sigs->buffer + sigs->count * 4, chunk_sig, 4);
    ++sigs->count;
    sigs->sorted = 0;
    return 0;
}
