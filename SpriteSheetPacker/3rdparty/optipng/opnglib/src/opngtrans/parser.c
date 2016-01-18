/*
 * opngtrans/parser.c
 * Object parser.
 *
 * Copyright (C) 2011-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include <ctype.h>
#include <string.h>

#define OPNGLIB_INTERNAL
#include "parser.h"
#include "trans.h"
#include "../opngcore/util.h"


static const struct
{
    const char *objname;
    opng_id_t id;
} object_id_map[] =
{
    /* Custom identifiers must not be groups of 4 letters. */
    { "all",                   OPNG_ID_ALL                   },
    { "image",                 OPNG_ID__RESERVED             },
    { "image.gray",            OPNG_ID__RESERVED             },
    { "image.grey",            OPNG_ID__RESERVED             },
    { "image.red",             OPNG_ID__RESERVED             },
    { "image.green",           OPNG_ID__RESERVED             },
    { "image.blue",            OPNG_ID__RESERVED             },
    { "image.rgb",             OPNG_ID__RESERVED             },
    { "image.alpha",           OPNG_ID_IMAGE_ALPHA           },
    { "image.precision",       OPNG_ID_IMAGE_PRECISION       },
    { "image.gray.precision",  OPNG_ID_IMAGE_GRAY_PRECISION  },
    { "image.grey.precision",  OPNG_ID_IMAGE_GRAY_PRECISION  },
    { "image.red.precision",   OPNG_ID_IMAGE_RED_PRECISION   },
    { "image.green.precision", OPNG_ID_IMAGE_GREEN_PRECISION },
    { "image.blue.precision",  OPNG_ID_IMAGE_BLUE_PRECISION  },
    { "image.rgb.precision",   OPNG_ID_IMAGE_RGB_PRECISION   },
    { "image.alpha.precision", OPNG_ID_IMAGE_ALPHA_PRECISION },
    { "image.luma.bt601",      OPNG_ID__RESERVED             },
    { "image.luma.bt709",      OPNG_ID__RESERVED             },
    { "image.luma",            OPNG_ID__RESERVED             },
    { "image.chroma.bt601",    OPNG_ID_IMAGE_CHROMA_BT601    },
    { "image.chroma.bt709",    OPNG_ID_IMAGE_CHROMA_BT709    },
    { "image.chroma",          OPNG_ID_IMAGE_CHROMA_BT709    },
    { "animation",             OPNG_ID_ANIMATION             },
    { NULL,                    0                             }
};


/*
 * Reads an object name from the given string.
 * A valid object has the form "[:alpha:]+(.[:alpha:]+)*".
 */
static int
opng_sscan_object(const char *str,
                  size_t *objname_offset_ptr,
                  size_t *objname_length_ptr,
                  int *is_chunk_name_ptr)
{
    const char *start;
    int valid;
    size_t i;

    /* Initialize the results. */
    *objname_offset_ptr = 0;
    *objname_length_ptr = 0;
    *is_chunk_name_ptr = 0;

    /* Skip the leading whitespace. */
    for (i = 0; isspace(str[i]); ++i)
    {
    }
    if (str[i] == 0)
        return 0;

    /* Scan and match '[A-Za-z][A-Za-z0-9]*(\.[A-Za-z][A-Za-z0-9]*)*' */
    start = str + i;
    *objname_offset_ptr = i;
    for ( ; ; )
    {
        valid = 0;
        if (opng_isdigit(str[i]))
            break;
        for ( ; opng_isalnum(str[i]); ++i)
            valid = 1;
        if (!valid)
            break;
        if (str[i] != '.')
            break;
        ++i;
    }
    if (!valid)
        return -1;

    /* Store the results. */
    *objname_length_ptr = i - *objname_offset_ptr;
    *is_chunk_name_ptr =
        (*objname_length_ptr == 4 &&
         opng_isalpha(start[0]) && opng_isalpha(start[1]) &&
         opng_isalpha(start[2]) && opng_isalpha(start[3]));
    return 1;
}

/*
 * Finds the object id that corresponds to the given object name.
 */
static opng_id_t
opng_object_id(const char *objname, size_t objname_length)
{
    size_t i;

    for (i = 0; object_id_map[i].objname != NULL; ++i)
    {
        if (strncmp(object_id_map[i].objname, objname, objname_length) == 0 &&
            object_id_map[i].objname[objname_length] == 0)
            return object_id_map[i].id;
    }
    return OPNG_ID__UNKNOWN;
}

/*
 * Converts a string representing an object to an object id.
 */
opng_id_t
opng_string_to_id(const char *str,
                  size_t *objname_offset_ptr,
                  size_t *objname_length_ptr)
{
    const char *objname;
    int is_chunk_name;

    if (opng_sscan_object(str, objname_offset_ptr, objname_length_ptr,
                          &is_chunk_name) <= 0)
        return OPNG_ID__NONE;

    objname = str + *objname_offset_ptr;
    if (!is_chunk_name)
        return opng_object_id(objname, *objname_length_ptr);

    /* Critical chunks and tRNS contain image data.
     * The other chunks contain metadata.
     */
    if (opng_isupper(objname[0]))
        return OPNG_ID_CHUNK_IMAGE;
    if (memcmp(objname, "tRNS", 4) == 0)
        return OPNG_ID_CHUNK_IMAGE;
    if (memcmp(objname, "acTL", 4) == 0 ||
        memcmp(objname, "fcTL", 4) == 0 ||
        memcmp(objname, "fdAT", 4) == 0)
        return OPNG_ID_CHUNK_ANIMATION;
    return OPNG_ID_CHUNK_META;
}

/*
 * Parses an input string in the form "object[,object...]" and adds
 * the results to the given id set and (if given) chunk signature set.
 */
int
opng_parse_objects(opng_id_t *ids_ptr,
                   struct opng_sigs *sigs,
                   const char *objnames,
                   opng_id_t accept_mask,
                   struct opng_parse_err_info *err_info_ptr)
{
    const char *ptr;
    opng_id_t id;
    size_t objname_rel_offset, objname_offset, objname_length;

    memset(err_info_ptr, 0, sizeof(*err_info_ptr));

    ptr = objnames;
    do
    {
        id = opng_string_to_id(ptr, &objname_rel_offset, &objname_length);
        *ids_ptr |= id;
        if ((id & OPNG_IDSET_CHUNK) != 0 && sigs != NULL)
            opng_sigs_add(sigs, ptr);
        ptr += objname_rel_offset;
        objname_offset = ptr - objnames;
        ptr += objname_length;
        while (isspace(*ptr))
            ++ptr;
        if (*ptr == ',' || *ptr == ';')
            ++ptr;
        else if (*ptr == 0)
            ptr = NULL;
        else
        {
            /* Syntax error. */
            err_info_ptr->id = OPNG_ID__NONE;
            return -1;
        }
        if ((id & accept_mask) == 0)
        {
            /* Missing or incorrect id. */
            err_info_ptr->id = id;
            if (objname_length > 0)
            {
                err_info_ptr->objname_offset = objname_offset;
                err_info_ptr->objname_length = objname_length;
            }
            return -1;
        }
    } while (ptr != NULL);
    return 0;
}

/*
 * Parses an input string in the form "object=value" and retrieves
 * the object id and value string.
 */
int
opng_parse_object_value(opng_id_t *id_ptr,
                        size_t *value_offset_ptr,
                        const char *objname_eq_value,
                        opng_id_t accept_mask,
                        struct opng_parse_err_info *err_info_ptr)
{
    const char *ptr;
    opng_id_t id;
    size_t objname_offset, objname_length;

    memset(err_info_ptr, 0, sizeof(*err_info_ptr));

    id = opng_string_to_id(objname_eq_value, &objname_offset, &objname_length);
    *id_ptr = id;
    ptr = objname_eq_value + objname_offset + objname_length;
    while (isspace(*ptr))
        ++ptr;
    if (*ptr == '=')
        ++ptr;
    else if (*ptr != 0)
    {
        /* Syntax error. */
        err_info_ptr->id = OPNG_ID__NONE;
        return -1;
    }
    if ((id & accept_mask) == 0)
    {
        /* Missing or incorrect id. */
        err_info_ptr->id = id;
        if (objname_length > 0)
        {
            err_info_ptr->objname_offset = objname_offset;
            err_info_ptr->objname_length = objname_length;
        }
        return -1;
    }
    *value_offset_ptr = ptr - objname_eq_value;
    return 0;
}

/*
 * Converts a numeric string representing a number to a smallint value.
 */
int
opng_string_to_smallint(const char *str)
{
    long result;
    char ch;

    if (sscanf(str, "%ld %c", &result, &ch) != 1)
        return -1;
    if (result < OPNG_SMALLINT_MIN)
        return OPNG_SMALLINT_MIN;
    if (result > OPNG_SMALLINT_MAX)
        return OPNG_SMALLINT_MAX;
    return (int)result;
}
