/**
 * @file opnglib/opnglib.h
 *
 * @brief
 * OPNGLIB is a PNG Library.
 *
 * @mainpage
 * @b OPNGLIB is the PNG optimization library used by @b optipng.
 * It consists of the following components:
 * @li @b OPNGCORE is a PNG Compression Optimization and Recovery Engine.
 * @li @b OPNGTRANS is a PNG Transformer.
 * @li @b OPTK is a Portable Tool Kit.
 *
 * @section Synopsis
 * @code
 * # see opngcore.h
 * optimizer = opng_create_optimizer()
 * defer: opng_destroy_optimizer(optimizer)
 * options = allocate opng_options
 * defer: deallocate options
 * populate options
 * opng_set_options(optimizer, options)
 * optional:
 *     # see opngtrans.h
 *     transformer = opng_create_transformer()
 *     defer: opng_destroy_transformer(transformer)
 *     populate transformer
 *     opng_set_transformer(optimizer, transformer)
 * for file in files:
 *     opng_optimize_file(optimizer, file)
 * @endcode
 *
 * @section Copyright
 * Copyright (C) 2001-2011 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file, or visit
 * http://www.opensource.org/licenses/zlib-license.php
 *
 * @section Caveat
 * @b OPNGLIB is not thread-safe: it is currently not possible
 * to create two or more optimizer objects in a process.
 **/

#ifndef OPNGLIB_OPNGLIB_H
#define OPNGLIB_OPNGLIB_H

#include "opngcore.h"
#include "opngtrans.h"

#endif  /* OPNGLIB_OPNGLIB_H */
