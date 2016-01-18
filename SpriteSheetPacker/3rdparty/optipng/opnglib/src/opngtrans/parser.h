/*
 * opngtrans/parser.h
 * Object parser.
 *
 * Copyright (C) 2011-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef OPNGTRANS_PARSER_H
#define OPNGTRANS_PARSER_H

#ifndef OPNGLIB_INTERNAL
#error This header, internal to opnglib, is not meant to be visible outside.
#endif

#include <stddef.h>

#include "trans.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * The parse error info structure.
 */
struct opng_parse_err_info
{
    size_t objname_offset;
    size_t objname_length;
    opng_id_t id;
};

/*
 * Converts a string representing an object to an object id.
 * Sets objname_offset and objname_length to indicate the exact
 * object name boundaries within the input string.
 * Returns the object id, or an appropriate error id in case of error.
 */
opng_id_t
opng_string_to_id(const char *str,
                  size_t *objname_offset_ptr,
                  size_t *objname_length_ptr);

/*
 * Parses an input string in the form "object[,object...]" and adds
 * the results to the given id set and (if given) chunk signature set.
 * Returns 0 on success or -1 on error.
 */
int
opng_parse_objects(opng_id_t *ids_ptr,
                   struct opng_sigs *sigs,
                   const char *names,
                   opng_id_t accept_mask,
                   struct opng_parse_err_info *err_info_ptr);

/*
 * Parses an input string in the form "object=value" and retrieves
 * the object id and value string.
 * Returns 0 on success or -1 on error.
 */
int
opng_parse_object_value(opng_id_t *id_ptr,
                        size_t *value_offset_ptr,
                        const char *name_eq_value,
                        opng_id_t accept_mask,
                        struct opng_parse_err_info *err_info_ptr);

/*
 * The smallint limits.
 */
enum
{
    OPNG_SMALLINT_MIN = 0,
    OPNG_SMALLINT_MAX = 32767
};

/*
 * Converts a numeric string representing a number to a smallint value.
 * Returns the converted value, possibly truncated to fit within the
 * smallint limits, or -1 in case of error.
 */
int
opng_string_to_smallint(const char *str);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPNGTRANS_PARSER_H */
