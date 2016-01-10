/*
 * opngtrans/chunksig.h
 * Chunk signatures.
 *
 * Copyright (C) 2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef OPNGTRANS_CHUNKSIG_H
#define OPNGTRANS_CHUNKSIG_H

#ifndef OPNGLIB_INTERNAL
#error This header, internal to opnglib, is not meant to be visible outside.
#endif

#include <stddef.h>

#include "png.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * The chunk signature set structure.
 *
 * This is a set structure optimized for the case when all insertions
 * are expected to be done first and all lookups are expected to be done
 * last.
 *
 * Since we are inside an internal module, we afford not to make
 * this structure opaque, e.g. to allow fast allocations on the stack.
 * The structure members, however, should not be accessed externally.
 *
 * The operations associated with this structure do NOT perform validity
 * checks on chunk signatures.
 *
 * TODO: Try replacing this with a hash, it might be better.
 */
struct opng_sigs
{
    png_byte *buffer;
    size_t count;
    size_t capacity;
    int sorted;
};

/*
 * The maximum size of the chunk signature set.
 *
 * This size is a very large number under typical PNG chunk handling
 * requirements, yet it limits the occupied memory to a reasonably small
 * limit.
 */
enum
{
    OPNG_SIGS_SIZE_MAX = 4096
};

/*
 * Initializes a chunk signature set.
 */
void
opng_sigs_init(struct opng_sigs *sigs);

/*
 * Clears a chunk signature set.
 */
void
opng_sigs_clear(struct opng_sigs *sigs);

/*
 * Sorts and uniq's the array buffer that stores chunk signatures.
 */
void
opng_sigs_sort_uniq(struct opng_sigs *sigs);

/*
 * Returns 1 if a chunk signature is found in the given set,
 * or 0 otherwise.
 */
int
opng_sigs_find(const struct opng_sigs *sigs, const png_byte *chunk_sig);

/*
 * Converts a chunk name to a chunk signature and adds it to
 * a chunk signature set.
 * (In the ASCII world, the conversion is a no-op.)
 * Returns 0 on success or -1 on failure.
 */
int
opng_sigs_add(struct opng_sigs *sigs, const char *chunk_name);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPNGTRANS_CHUNKSIG_H */
