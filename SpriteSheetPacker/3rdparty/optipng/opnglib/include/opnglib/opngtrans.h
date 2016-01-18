/**
 * @file opnglib/opngtrans.h
 *
 * @brief
 * OPNGTRANS is a PNG Transformer.
 *
 * Copyright (C) 2001-2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file, or visit
 * http://www.opensource.org/licenses/zlib-license.php
 **/

#ifndef OPNGLIB_OPNGTRANS_H
#define OPNGLIB_OPNGTRANS_H

#include "opngcore.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * The transformer type.
 *
 * A transformer object can be used to apply transformations to
 * the image properties or to the image contents.
 *
 * Currently-supported transformations are: erasing channels
 * (alpha channel only) and stripping metadata.
 *
 * Other transformations, hopefully to be implemented in some future,
 * are: erasing other channels (e.g. chroma), altering the sample
 * precision (e.g. 16-to-8 bits for all channels, any-to-1 bit for
 * the alpha channel), etc.
 **/
typedef struct opng_transformer /*opaque*/ opng_transformer_t;

/**
 * Creates a transformer object.
 * @return the transformer created, or @c NULL on failure.
 **/
opng_transformer_t *
opng_create_transformer(void);

/**
 * Sets the transformer in an optimizer object.
 * @param optimizer
 *        the optimizer object whose transformer is changed.
 *        By default, the optimizer has a @c NULL transformer.
 * @param transformer
 *        the transformer to be changed.
 *        It can be @c NULL, which means that the optimized images
 *        are not altered.
 **/
void
opng_set_transformer(opng_optimizer_t *optimizer,
                     opng_transformer_t *transformer);

/**
 * Specifies an object to be set to a given value by the given transformer.
 * Only image data objects can be reset.
 * @param transformer
 *        the transformer object.
 * @param object_name_eq_value
 *        an expression in the form "name=value".
 * @param err_objname_offset_ptr
 *        in case of failure, shall point to the erroneous object within
 *        @c object_name_eq_value.
 * @param err_objname_length_ptr
 *        in case of failure, shall store the length of the erroneous object.
 * @param err_message_ptr
 *        in case of failure, may point to an error message.
 * @return 0 on success, or -1 on failure.
 **/
int
opng_transform_set_object(opng_transformer_t *transformer, 
                          const char *object_name_eq_value,
                          size_t *err_objname_offset_ptr,
                          size_t *err_objname_length_ptr,
                          const char **err_message_ptr);

/**
 * Specifies a list of objects to be reset by the given transformer.
 * This list of objects will be added to (not replace) any previously-specified
 * list of objects.
 * Only image data objects can be reset.
 * @param transformer
 *        the transformer object.
 * @param object_names
 *        a comma- or semicolon-separated enumeration of object names.
 * @param err_objname_offset_ptr
 *        in case of failure, shall point to the erroneous object within
 *        @c object_names.
 * @param err_objname_length_ptr
 *        in case of failure, shall store the length of the erroneous object.
 * @param err_message_ptr
 *        in case of failure, may point to an error message.
 * @return 0 on success, or -1 on failure.
 **/
int
opng_transform_reset_objects(opng_transformer_t *transformer, 
                             const char *object_names,
                             size_t *err_objname_offset_ptr,
                             size_t *err_objname_length_ptr,
                             const char **err_message_ptr);

/**
 * Specifies a list of objects to be stripped by the given transformer.
 * These objects will be added to (not replace) any previously-specified
 * list of objects.
 * Only metadata objects can be stripped.
 * @param transformer
 *        the transformer object.
 * @param object_names
 *        a comma- or semicolon-separated enumeration of object names.
 * @param err_objname_offset_ptr
 *        in case of failure, shall point to the erroneous object within
 *        @c object_names.
 * @param err_objname_length_ptr
 *        in case of failure, shall store the length of the erroneous object.
 * @param err_message_ptr
 *        in case of failure, may point to an error message.
 * @return 0 on success, or -1 on failure.
 **/
int
opng_transform_strip_objects(opng_transformer_t *transformer,
                             const char *object_names,
                             size_t *err_objname_offset_ptr,
                             size_t *err_objname_length_ptr,
                             const char **err_message_ptr);

/**
 * Specifies a list of objects to be protected (i.e. not stripped) by
 * the given transformer.
 * For example, it is possible to strip all metadata, but protect
 * @c sRGB and @c iCCP.
 * These objects will be added to (not replace) any previously-specified
 * list of objects.
 * Only metadata objects can be protected.
 * @param transformer
 *        the transformer object.
 * @param object_names
 *        a comma- or semicolon-separated enumeration of object names.
 * @param err_objname_offset_ptr
 *        in case of failure, shall point to the erroneous object within
 *        @c object_names.
 * @param err_objname_length_ptr
 *        in case of failure, shall store the length of the erroneous object.
 * @param err_message_ptr
 *        in case of failure, may point to an error message.
 * @return 0 on success, or -1 on failure.
 **/
int
opng_transform_protect_objects(opng_transformer_t *transformer,
                               const char *object_names,
                               size_t *err_objname_offset_ptr,
                               size_t *err_objname_length_ptr,
                               const char **err_message_ptr);

/**
 * Seals a transformer object.
 * @param transformer
 *        the transformer object to be sealed.
 * @return the sealed transformer.
 **/
const opng_transformer_t *
opng_seal_transformer(opng_transformer_t *transformer);

/**
 * Destroys a transformer object.
 * @param transformer
 *        the transformer object to be destroyed.
 **/
void
opng_destroy_transformer(opng_transformer_t *transformer);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPNGLIB_OPNGTRANS_H */
