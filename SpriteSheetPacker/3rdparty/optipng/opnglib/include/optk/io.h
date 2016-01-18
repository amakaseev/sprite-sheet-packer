/*
 * optk/io.h
 * I/O utilities.
 *
 * Copyright (C) 2003-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#ifndef OPTK_IO_H_
#define OPTK_IO_H_

#include <stdio.h>
#include "integer.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * File translation modes.
 * These symbols are used by optk_fsetmode().
 */
#define OPTK_FMODE_BINARY  0x0000
#define OPTK_FMODE_TEXT    0x0001
#define OPTK_FMODE_WTEXT   0x0002
#define OPTK_FMODE_U8TEXT  0x0008
#define OPTK_FMODE_U16TEXT 0x0010
#define OPTK_FMODE_U32TEXT 0x0020

/*
 * The file offset type.
 * This is a signed integer type that is at least 64-bits wide.
 */
typedef optk_int64_t optk_foffset_t;
#define OPTK_FOFFSET_MIN  OPTK_INT64_MIN
#define OPTK_FOFFSET_MAX  OPTK_INT64_MAX
#define OPTK_FOFFSET_SCNd OPTK_SCNd64
#define OPTK_FOFFSET_SCNi OPTK_SCNi64
#define OPTK_FOFFSET_SCNo OPTK_SCNo64
#define OPTK_FOFFSET_SCNx OPTK_SCNx64
#define OPTK_FOFFSET_PRId OPTK_PRId64
#define OPTK_FOFFSET_PRIi OPTK_PRIi64
#define OPTK_FOFFSET_PRIo OPTK_PRIo64
#define OPTK_FOFFSET_PRIx OPTK_PRIx64
#define OPTK_FOFFSET_PRIX OPTK_PRIX64

/*
 * The file size type.
 * This is an unsigned integer type that is at least 64-bits wide.
 */
typedef optk_uint64_t optk_fsize_t;
#define OPTK_FSIZE_MAX  OPTK_UINT64_MAX
#define OPTK_FSIZE_SCNu OPTK_SCNu64
#define OPTK_FSIZE_SCNo OPTK_SCNo64
#define OPTK_FSIZE_SCNx OPTK_SCNx64
#define OPTK_FSIZE_PRIu OPTK_PRIu64
#define OPTK_FSIZE_PRIo OPTK_PRIo64
#define OPTK_FSIZE_PRIx OPTK_PRIx64
#define OPTK_FSIZE_PRIX OPTK_PRIX64

/*
 * Returns the current value of the file position indicator.
 * On error, the function returns (osys_foffset_t)(-1).
 */
optk_foffset_t
optk_ftello(FILE *stream);

/*
 * Sets the file position indicator at the specified file offset.
 * On success, the function returns 0. On error, it returns -1.
 */
int
optk_fseeko(FILE *stream, optk_foffset_t offset, int whence);

/*
 * Gets the size of the specified file stream.
 * This function may change the file position indicator.
 * On success, the function returns 0. On error, it returns -1.
 */
int
optk_fgetsize(FILE *stream, optk_fsize_t *size);

/*
 * Reads a block of data from the specified file offset.
 * The file-position indicator is saved and restored after reading.
 * The file buffer is flushed before and after reading.
 * On success, the function returns the number of bytes read.
 * On error, it returns 0.
 */
size_t
optk_fread_at(FILE *stream, long offset, int whence,
              void *block, size_t blocksize);

/*
 * Writes a block of data at the specified file offset.
 * The file-position indicator is saved and restored after writing.
 * The file buffer is flushed before and after writing.
 * On success, the function returns the number of bytes written.
 * On error, it returns 0.
 */
size_t
optk_fwrite_at(FILE *stream, long offset, int whence,
               const void *block, size_t blocksize);

/*
 * Sets the translation mode of the specified file stream to the
 * specified mode, which can be one of the OPTK_FMODE_ constants.
 * On success, the function returns 0.
 * On error, it returns -1.
 */
int
optk_fsetmode(FILE *stream, int fmode);

/*
 * Checks if the specified file stream is associated with a terminal device.
 * The function returns one of the following values:
 *    1: the specified file is associated with a terminal device.
 *    0: the specified file is not associated with a terminal device.
 *   -1: the implementation is unable to perform this check.
 */
int
optk_ftty(FILE *stream);

/*
 * Extracts the base component of a path name.
 * The function returns a pointer to the base component within
 * the given path name. The length of the base component string
 * is stored into basename_len, if basename_len is not NULL.
 */
char *
optk_get_basename(const char *path, size_t *basename_len);

/*
 * Extracts the directory component of a path name.
 * The function returns a pointer to the directory component within
 * the given path name. The length of the directory component string
 * is stored into dirname_len, if dirname_len is not NULL.
 * If with_dir_separator is non-zero, the directory separator is
 * included in the result (e.g. "examples/").
 */
char *
optk_get_dirname(const char *path, int with_dir_separator,
                 size_t *dirname_len);

/*
 * Creates a path name by changing the directory component of a given
 * path name.
 * The new directory name may optionally contain the directory separator.
 * If the given buffer is non-NULL and sufficiently large to hold
 * the resulting path, the resulting path is stored into the buffer.
 * The function returns the length of the resulting path (excluding the
 * terminating '\0'), regardless whether the buffer is written or not.
 */
size_t
optk_set_dirname(char *buffer, size_t bufsize, const char *path,
                 const char *new_dirname);

/*
 * Extracts the extension component of a path name.
 * The function returns a pointer to the extension component within
 * the given path name. The length of the extension component string
 * is stored into extname_len, if extname_len is not NULL.
 * If with_ext_separator is non-zero, the extension separator is
 * included in the result (e.g. ".gz").
 * The number num_ext indicates the maximum number of extensions to
 * be returned; -1 indicates all extensions.
 */
char *
optk_get_extname(const char *path, int with_ext_separator, int num_ext,
                 size_t *extname_len);

/*
 * Creates a path name by changing the extension component of a given
 * path name.
 * The number num_ext indicates the maximum number of extensions in the
 * original path (or -1 for all extensions) to be replaced.
 * The new extension name may optionally contain the extension separator.
 * The new extension can be the NULL, indicating that the resulting
 * path has neither an extension nor an extension separator.
 * Otherwise, the new path will always contain the extension separator
 * (even when the new extension name is the empty string).
 * If the given buffer is non-NULL and sufficiently large to hold
 * the resulting path, the resulting path is stored into the buffer.
 * The function returns the length of the resulting path (excluding the
 * terminating '\0'), regardless whether the buffer is written or not.
 */
size_t
optk_set_extname(char *buffer, size_t bufsize, const char *path, int num_ext,
                 const char *new_extname);

/*
 * Copies the source file onto the destination file path.
 * On success, the function returns 0.
 * On error, it returns -1.
 */
int
optk_copy(const char *src_path, const char *dest_path, int clobber);

/*
 * Moves the source file onto the destination file path.
 * On success, the function returns 0.
 * On error, it returns -1.
 */
int
optk_move(const char *src_path, const char *dest_path, int clobber);

/*
 * Changes the name of a file path.
 * On success, the function returns 0.
 * On error, it returns -1.
 */
int
optk_rename(const char *src_path, const char *dest_path, int clobber);

/*
 * Copies the attributes (access mode, time stamp, etc.) of the source
 * file path onto the destination file path.
 * On success, the function returns 0.
 * On error, it returns -1.
 */
int
optk_copy_attr(const char *src_path, const char *dest_path);

/*
 * Creates a new directory.
 * If the directory is successfully created, the function returns 1.
 * If a directory with the same name already exists, the function returns 0.
 * On error, the function returns -1.
 */
int
optk_create_dir(const char *dirname);

/*
 * Determines if the accessibility of the specified path satisfies
 * the specified access mode. The access mode consists of one or more
 * characters that indicate the checks to be performed, as follows:
 *   'e': the path exists (default).
 *   'f': the path exists and is a regular file.
 *   'd': the path exists and is a directory.
 *   'r': the path exists and read permission is granted.
 *   'w': the path exists and write permission is granted.
 *   'x': the path exists and execute permission is granted.
 * For example, to determine if a file can be opened for reading using
 * fopen(), use "fr" in the access mode.
 * If all checks succeed, the function returns 1.
 * If at least one check fails, the function returns 0.
 * If at least one check cannot be performed, the function returns -1.
 */
int
optk_test(const char *path, const char *mode);

/*
 * Determines if a path is an accessible directory.
 * This function has a behavior similar to optk_test(path, "d"), but
 * it might do a better handling of special paths like current dir,
 * drive name, path ending with dir separator, etc.
 * If the path is an accessible directory, the function returns 1.
 * If the path is not an accessible directory, the function returns 0.
 * If the check cannot be performed, the function returns -1.
 */
int
optk_test_dir(const char *path);

/*
 * Determines if two accessible paths are equivalent.
 * Two paths are said to be equivalent if they point to the same
 * physical location (e.g. on the disk).
 * If the two paths are equivalent, the function returns 1.
 * If the two paths are not equivalent, the function returns 0.
 * If at least one path is not accessible or does not exist, or
 * if the check cannot be performed, the function returns -1.
 */
int
optk_test_eq(const char *path1, const char *path2);

/*
 * Removes a directory entry.
 * On success, the function returns 0.
 * On error, it returns -1.
 */
int
optk_unlink(const char *path);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPTK_IO_H_ */
