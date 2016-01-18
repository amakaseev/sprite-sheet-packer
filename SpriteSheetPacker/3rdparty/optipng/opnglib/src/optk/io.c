/*
 * optk/io.c
 * I/O utilities.
 *
 * Copyright (C) 2003-2012 Cosmin Truta.
 *
 * This software is distributed under the zlib license.
 * Please see the accompanying LICENSE file.
 */

#include "io.h"
#include "integer.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * Auto-configuration.
 */
#if defined UNIX || defined __UNIX__ || defined __unix || defined __unix__ || \
    defined _BSD_SOURCE || defined _SVID_SOURCE || defined _GNU_SOURCE || \
    defined _POSIX_SOURCE || defined _POSIX_C_SOURCE || defined _XOPEN_SOURCE
#  define OPTK_OS_UNIX
/* To be continued. */
#endif

#if defined WIN32 || defined _WIN32 || defined _WIN32_WCE || \
    defined __WIN32__ || defined __NT__
#  define OPTK_OS_WIN32
#endif

#if defined WIN64 || defined _WIN64 || defined __WIN64__
#  define OPTK_OS_WIN64
#endif

#if defined WINDOWS || defined OPTK_OS_WIN32 || defined OPTK_OS_WIN64
#  define OPTK_OS_WINDOWS
#endif

#if defined DOS || defined _DOS || defined __DOS__ || \
    defined MSDOS || defined _MSDOS || defined __MSDOS__
#  define OPTK_OS_DOS
#endif

#if defined OS2 || defined OS_2 || defined __OS2__
#  define OPTK_OS_OS2
#endif

#if defined OPTK_OS_DOS || defined OPTK_OS_OS2
#  define OPTK_OS_DOSISH
#endif

#if defined __APPLE__
#  define OPTK_OS_MACOS
#  if defined __MACH__
#    define OPTK_OS_MACOSX
#    ifndef OPTK_OS_UNIX
#      define OPTK_OS_UNIX
#    endif
#  endif
#endif

#if defined __CYGWIN__ || defined __DJGPP__ || defined __EMX__
#  define OPTK_OS_UNIXISH
   /* Strictly speaking, this is not entirely correct, but "it works". */
#  ifndef OPTK_OS_UNIX
#    define OPTK_OS_UNIX
#  endif
#endif

#if defined OPTK_OS_UNIX || \
   (!defined OPTK_OS_WINDOWS && !defined OPTK_OS_DOSISH)
#  include <unistd.h>
#  if defined _POSIX_VERSION || defined _XOPEN_VERSION
#    ifndef OPTK_OS_UNIX
#      define OPTK_OS_UNIX
#    endif
#  endif
#endif

#if defined OPTK_OS_UNIX
#  define _BSD_SOURCE 1
#  include <strings.h>
#endif

#if defined OPTK_OS_UNIX || defined OPTK_OS_WINDOWS || defined OPTK_OS_DOSISH
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  ifdef _MSC_VER
#    include <sys/utime.h>
#  else
#    include <utime.h>
#  endif
#endif

#if defined OPTK_OS_WINDOWS || defined OPTK_OS_DOSISH || defined OPTK_OS_UNIXISH
#  include <io.h>
#endif

#if defined OPTK_OS_WINDOWS
#  include <windows.h>
#endif

#if defined OPTK_OS_DOSISH
#  include <process.h>
#endif

/*
 * More auto-configuration.
 */
#if (defined OPTK_OS_WINDOWS || defined OPTK_OS_DOSISH) && !defined OPTK_OS_UNIXISH
#  define OPTK_PATH_PATHSEP '\\'
#  define OPTK_PATH_PATHSEP_STR "\\"
#  define OPTK_PATH_PATHSEP_ALL_STR "/\\"
#else
#  define OPTK_PATH_PATHSEP '/'
#  define OPTK_PATH_PATHSEP_STR "/"
#  if defined OPTK_OS_UNIXISH
#    define OPTK_PATH_PATHSEP_ALL_STR "/\\"
#  elif defined OPTK_OS_MACOSX
#    define OPTK_PATH_PATHSEP_ALL_STR "/:"
#  else
#    define OPTK_PATH_PATHSEP_ALL_STR "/"
#  endif
#endif
#define OPTK_PATH_EXTSEP '.'
#define OPTK_PATH_EXTSEP_STR "."
/* TODO: Support more systems (e.g. OPTK_OS_RISCOS). */

#if defined OPTK_OS_WINDOWS || defined OPTK_OS_DOSISH || defined OPTK_OS_UNIXISH
#  define OPTK_PATH_DOS
#endif

#ifdef R_OK
#  define OPTK_TEST_READ R_OK
#else
#  define OPTK_TEST_READ 4
#endif
#ifdef W_OK
#  define OPTK_TEST_WRITE W_OK
#else
#  define OPTK_TEST_WRITE 2
#endif
#ifdef X_OK
#  define OPTK_TEST_EXEC X_OK
#else
#  define OPTK_TEST_EXEC 1
#endif
#ifdef F_OK
#  define OPTK_TEST_FILE F_OK
#else
#  define OPTK_TEST_FILE 0
#endif

/*
 * Utility macros.
 */
#define OPTK_UNUSED(x) ((void)(x))

#ifdef OPTK_PATH_DOS
#  define OPTK_PATH_IS_DRIVE_LETTER(ch) \
          (((ch) >= 'A' && (ch) <= 'Z') || ((ch) >= 'a' && (ch) <= 'z'))
#endif

#ifdef OPTK_OS_WINDOWS
#  if !defined OPTK_OS_WIN64
#    define OPTK_OS_WINDOWS_IS_WIN9X() (GetVersion() >= 0x80000000U)
#  else
#    define OPTK_OS_WINDOWS_IS_WIN9X() 0
#  endif
#  if (defined _MSC_VER && _MSC_VER-0 >= 1400) || \
      (defined __MSVCRT_VERSION__ && __MSVCRT_VERSION__-0 >= 0x800)
#    define OPTK_HAVE_STDIO__I64
#  endif
#endif


/*
 * Returns the current value of the file position indicator.
 */
optk_foffset_t
optk_ftello(FILE *stream)
{
#if defined OPTK_OS_WINDOWS && defined OPTK_HAVE_STDIO__I64

    return (optk_foffset_t)_ftelli64(stream);

#elif defined OPTK_OS_UNIX && (OPTK_FOFFSET_MAX > LONG_MAX)

    /* We don't know if off_t is sufficiently wide, we only know that
     * long isn't. We are trying just a little harder, in the absence
     * of an fopen64/ftell64 solution.
     */
    return (optk_foffset_t)ftello(stream);

#else  /* generic */

    return (optk_foffset_t)ftell(stream);

#endif
}

/*
 * Sets the file position indicator at the specified file offset.
 */
int
optk_fseeko(FILE *stream, optk_foffset_t offset, int whence)
{
#if defined OPTK_OS_WINDOWS

#if defined OPTK_HAVE_STDIO__I64
    return _fseeki64(stream, (__int64)offset, whence);
#else
    return fseek(stream, (long)offset, whence);
#endif

#elif defined OPTK_OS_UNIX

#if OPTK_FOFFSET_MAX > LONG_MAX
    /* We don't know if off_t is sufficiently wide, we only know that
     * long isn't. We are trying just a little harder, in the absence
     * of an fopen64/fseek64 solution.
     */
    return fseeko(stream, (off_t)offset, whence);
#else
    return fseek(stream, (long)offset, whence);
#endif

#else  /* generic */

    return (fseek(stream, (long)offset, whence) == 0) ? 0 : -1;

#endif
}

/*
 * Gets the size of the specified file stream.
 */
int
optk_fgetsize(FILE *stream, optk_fsize_t *size)
{
#if defined OPTK_OS_WINDOWS

    __int64 length;

    length = _filelengthi64(_fileno(stream));
    if (length == -1)
        return -1;
    *size = (optk_fsize_t)length;
    return 0;

#elif defined OPTK_OS_UNIX

    struct stat sbuf;

    if (fstat(fileno(stream), &sbuf) != 0)
        return -1;
    *size = (optk_fsize_t)sbuf.st_size;
    return 0;

#else  /* generic */

    optk_foffset_t crt, end;

    crt = optk_ftello(stream);
    if (crt == -1)
        return -1;
    if (optk_fseeko(stream, 0, SEEK_END) != 0)
        return -1;
    end = optk_ftello(stream);
    if (offset == -1)
        return -1;
    *size = (optk_fsize_t)end;
    if (optk_fseeko(stream, crt, SEEK_SET) != 0)
        return -1;
    return 0;

#endif
}

/*
 * Reads a block of data from the specified file offset.
 */
size_t
optk_fread_at(FILE *stream, long offset, int whence,
              void *block, size_t blocksize)
{
    fpos_t pos;
    size_t result;

    if (fgetpos(stream, &pos) != 0)
        return 0;
    if (fseek(stream, offset, whence) == 0)
        result = fread(block, 1, blocksize, stream);
    else
        result = 0;
    if (fsetpos(stream, &pos) != 0)
        result = 0;
    return result;
}

/*
 * Writes a block of data at the specified file offset.
 */
size_t
optk_fwrite_at(FILE *stream, long offset, int whence,
               const void *block, size_t blocksize)
{
    fpos_t pos;
    size_t result;

    if (fgetpos(stream, &pos) != 0 || fflush(stream) != 0)
        return 0;
    if (fseek(stream, offset, whence) == 0)
        result = fwrite(block, 1, blocksize, stream);
    else
        result = 0;
    if (fflush(stream) != 0)
        result = 0;
    if (fsetpos(stream, &pos) != 0)
        result = 0;
    return result;
}

/*
 * Sets the translation mode of the specified file stream to the
 * specified mode.
 */
int
optk_fsetmode(FILE *stream, int fmode)
{
#if defined _O_BINARY

    int o_mode;

    switch (fmode)
    {
    case OPTK_FMODE_BINARY:
        o_mode = _O_BINARY;
        break;
    case OPTK_FMODE_TEXT:
        o_mode = _O_TEXT;
        break;
#   ifdef _O_WTEXT
    case OPTK_FMODE_WTEXT:
        o_mode = _O_WTEXT;
        break;
#   endif
#   ifdef _O_U8TEXT
    case OPTK_FMODE_U8TEXT:
        o_mode = _O_U8TEXT;
        break;
#   endif
#   ifdef _O_U16TEXT
    case OPTK_FMODE_U16TEXT:
        o_mode = _O_U16TEXT;
        break;
#   endif
#   ifdef _O_U32TEXT
    case OPTK_FMODE_U32TEXT:
        o_mode = _O_U32TEXT;
        break;
#   endif
    default:
        return -1;  /* not handled */
    }

    return _setmode(_fileno(stream), o_mode);

#elif defined O_BINARY

    int o_mode;

    switch (fmode)
    {
    case OPTK_FMODE_BINARY:
        o_mode = O_BINARY;
        break;
    case OPTK_FMODE_TEXT:
        o_mode = O_TEXT;
        break;
    default:
        return -1;  /* not handled */
    }

    return setmode(fileno(stream), o_mode);

#else  /* Unix and others */

    OPTK_UNUSED(stream);
    OPTK_UNUSED(fmode);
    return 0;  /* nothing to do */

#endif
}

/*
 * Checks if the specified file stream is associated with a terminal device.
 */
int
optk_ftty(FILE *stream)
{
#if defined OPTK_OS_WINDOWS

    return _isatty(_fileno(stream));

#elif defined OPTK_OS_UNIX || defined OPTK_OS_DOSISH

    return isatty(fileno(stream));

#else  /* generic */

    OPTK_UNUSED(stream);
    return -1;

#endif
}

/*
 * Extracts the base component of a path name.
 */
char *
optk_get_basename(const char *path, size_t *basename_len)
{
    /*
     * The current implementation works for Unix-like path names only,
     * where the base component is at the end of the path name, following
     * the directory component.
     */

    const char *basename, *ptr;

    if (path == NULL)
        return NULL;

    basename = ptr = path;
#ifdef OPTK_PATH_DOS
    /* Skip the drive component, if any. */
    if (basename[1] == ':' && OPTK_PATH_IS_DRIVE_LETTER(basename[0]))
        basename += 2;
#endif
    /* Skip the directory component, if any. */
    for ( ; ; )
    {
        ptr = strpbrk(basename, OPTK_PATH_PATHSEP_ALL_STR);
        if (ptr == NULL)
            break;
        basename = ptr + 1;
    }

    if (basename_len != NULL)
        *basename_len = strlen(basename);
    return (char *)basename;
}

/*
 * Extracts the directory component of a path name.
 */
char *
optk_get_dirname(const char *path, int with_dir_separator,
                 size_t *dirname_len)
{
    /*
     * The current implementation works for Unix-like path names only,
     * where the directory component is at the beginning of the path name.
     */

    const char *dirend;

    if (path == NULL || dirname_len == NULL)
        return (char *)path;

    /* The directory name ends at the base name. */
    dirend = optk_get_basename(path, NULL);
    if (!with_dir_separator)
    {
        while (dirend > path)
        {
            if (strchr(OPTK_PATH_PATHSEP_ALL_STR, *(dirend - 1)) == NULL)
                break;
            --dirend;
        }
    }

    *dirname_len = dirend - path;
    return (char *)path;
}

/*
 * Creates a path name by changing the directory component of a given
 * path name.
 */
size_t
optk_set_dirname(char *buffer, size_t bufsize, const char *path,
                 const char *new_dirname)
{
    /*
     * The current implementation works for Unix-like path names only,
     * where the directory component is at the beginning of the path name.
     */

    const char *basename, *dirname;
    size_t baselen, new_dirlen, outlen;
    int append_slash;

    /* Extract the base name from the path. */
    if (path != NULL)
        basename = optk_get_basename(path, &baselen);
    else
    {
        basename = "";
        baselen = 0;
    }

    /* Get the directory name. */
    append_slash = 0;
    if (new_dirname != NULL)
    {
        dirname = new_dirname;
        new_dirlen = strlen(dirname);
        if (new_dirlen > 0)
        {
#ifdef OPTK_PATH_DOS
            if (new_dirlen == 2 &&
                dirname[1] == ':' && OPTK_PATH_IS_DRIVE_LETTER(dirname[0]))
            { /* do not replace "C:" with "C:\" */ }
            else
#endif
            if (strchr(OPTK_PATH_PATHSEP_ALL_STR,
                       dirname[new_dirlen - 1]) == NULL)
                append_slash = 1;
        }
    }
    else
    {
        dirname = "";
        new_dirlen = 0;
    }

    /* Store the new path and return the result. */
    outlen = new_dirlen + append_slash + baselen;
    if (buffer != NULL && bufsize > outlen)
    {
        memcpy(buffer, dirname, new_dirlen);
        if (append_slash)
            buffer[new_dirlen++] = OPTK_PATH_PATHSEP;
        strcpy(buffer + new_dirlen, basename);
    }
    return outlen;
}

/*
 * Extracts the extension component of a path name.
 */
char *
optk_get_extname(const char *path, int with_ext_separator, int num_ext,
                 size_t *extname_len)
{
    /*
     * The current implementation works for Unix-like path names only,
     * where the extension component is at the end of the base component.
     */

    size_t pos, last_pos, len, i;

    if (path == NULL)
        return NULL;

    /* The extension name starts with '.' inside the base name. */
    last_pos = strlen(path);
    len = 0;
    i = (num_ext == 0) ? 0 : last_pos;
    while (i > 0)
    {
        if (path[--i] == OPTK_PATH_EXTSEP)
        {
            pos = i + (with_ext_separator ? 0 : 1);
            len += (last_pos - pos);
            last_pos = pos;
            if (--num_ext == 0)
                break;
        }
        if (strchr(OPTK_PATH_PATHSEP_ALL_STR, path[i]) != NULL)
            break;
    }

    if (extname_len != NULL)
        *extname_len = len;
    return (char *)path + last_pos;
}

/*
 * Creates a path name by changing the extension component of a given
 * path name.
 */
size_t
optk_set_extname(char *buffer, size_t bufsize, const char *path, int num_ext,
                 const char *new_extname)
{
    /*
     * The current implementation works for Unix-like path names only,
     * where the extension component is at the end of the base component.
     */

    const char *old_extname, *extname;
    size_t extname_pos, new_extlen, outlen;
    int prepend_dot;

    /* Get the position of the old extension in path. */
    if (path != NULL)
    {
        old_extname = optk_get_extname(path, 1, num_ext, NULL);
        extname_pos = (old_extname >= path) ? (old_extname - path) : 0;
    }
    else
    {
        path = "";
        extname_pos = 0;
    }

    /* Get the new extension name. */
    prepend_dot = 0;
    if (new_extname != NULL)
    {
        extname = new_extname;
        new_extlen = strlen(extname);
        if (extname[0] != OPTK_PATH_EXTSEP)
            prepend_dot = 1;
    }
    else
    {
        extname = "";
        new_extlen = 0;
    }

    /* Store the new path and return the result. */
    outlen = extname_pos + prepend_dot + new_extlen;
    if (buffer != NULL && bufsize > outlen)
    {
        memcpy(buffer, path, extname_pos);
        if (prepend_dot)
            buffer[extname_pos++] = OPTK_PATH_EXTSEP;
        strcpy(buffer + extname_pos, extname);
    }
    return outlen;
}

/*
 * Copies the source file onto the destination file path.
 */
int
optk_copy(const char *src_path, const char *dest_path, int clobber);
/* TODO */

/*
 * Moves the source file onto the destination file path.
 */
int
optk_move(const char *src_path, const char *dest_path, int clobber);
/* TODO */

/*
 * Changes the name of a file path.
 */
int
optk_rename(const char *src_path, const char *dest_path, int clobber)
{
#if defined OPTK_OS_WINDOWS

    DWORD dwFlags;

#if !defined OPTK_OS_WIN64
    if (OPTK_OS_WINDOWS_IS_WIN9X())
    {
        /* MoveFileEx is not available under Win9X; use MoveFile. */
        if (MoveFileA(src_path, dest_path))
            return 0;
        if (!clobber)
            return -1;
        DeleteFileA(dest_path);
        return MoveFileA(src_path, dest_path) ? 0 : -1;
    }
#endif

    dwFlags = clobber ? MOVEFILE_REPLACE_EXISTING : 0;
    return MoveFileExA(src_path, dest_path, dwFlags) ? 0 : -1;

#elif defined OPTK_OS_UNIX

    if (!clobber)
    {
        if (access(dest_path, OPTK_TEST_FILE) >= 0)
            return -1;
    }
    return rename(src_path, dest_path);

#else  /* generic */

    if (optk_test(dest_path, "e") > 0)
    {
        if (!clobber)
            return -1;
        optk_unlink(dest_path);
    }
    return rename(src_path, dest_path);

#endif
}

/*
 * Copies the attributes (access mode, time stamp, etc.) of the source
 * file path onto the destination file path.
 */
int
optk_copy_attr(const char *src_path, const char *dest_path)
{
#if defined OPTK_OS_WINDOWS

    HANDLE hFile;
    FILETIME ftLastWrite;
    DWORD dwFlagsAndAttributes;
    BOOL result;

    hFile = CreateFileA(src_path, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return -1;
    result = GetFileTime(hFile, NULL, NULL, &ftLastWrite);
    CloseHandle(hFile);
    if (!result)
        return -1;

    dwFlagsAndAttributes =
        (OPTK_OS_WINDOWS_IS_WIN9X() ? 0 : FILE_FLAG_BACKUP_SEMANTICS);
    hFile = CreateFileA(dest_path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                        dwFlagsAndAttributes, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return -1;
    result = SetFileTime(hFile, NULL, NULL, &ftLastWrite);
    CloseHandle(hFile);
    if (!result)
        return -1;

    /* TODO: Copy the access mode. */

    return 0;

#elif defined OPTK_OS_UNIX || defined OPTK_OS_DOSISH

    struct stat sbuf;
    int /* mode_t */ mode;

    if (stat(src_path, &sbuf) != 0)
        return -1;

    mode = (int)sbuf.st_mode;
    if (chmod(dest_path, mode) != 0)
        return -1;

#if defined(AT_FDCWD) && !defined(__APPLE__) && !defined(__SVR4) && !defined(__sun)
    {
        struct timespec times[2];

        times[0] = sbuf.st_atim;
        times[1] = sbuf.st_mtim;
        if (utimensat(AT_FDCWD, dest_path, times, 0) != 0)
            return -1;
    }
#else  /* legacy utime */
    {
        struct utimbuf utbuf;

        utbuf.actime = sbuf.st_atime;
        utbuf.modtime = sbuf.st_mtime;
        if (utime(dest_path, &utbuf) != 0)
            return -1;
    }
#endif

    return 0;

#else  /* generic */

    OPTK_UNUSED(src_path);
    OPTK_UNUSED(dest_path);
    return 0;

#endif
}

/*
 * Creates a new directory.
 */
int
optk_create_dir(const char *dirname)
{
    if (optk_test_dir(dirname) > 0)
        return 0;

#if defined OPTK_OS_WINDOWS

    return CreateDirectoryA(dirname, NULL) ? 1 : -1;

#elif defined OPTK_OS_UNIX

    return (mkdir(dirname, 0777) == 0) ? 1 : -1;

#elif defined OPTK_OS_DOSISH

    return (mkdir(dirname) == 0) ? 1 : -1;

#else  /* generic */

    OPTK_UNUSED(dirname);
    return 0;

#endif
}

/*
 * Determines if the accessibility of the specified path satisfies
 * the specified access mode.
 */
int
optk_test(const char *path, const char *mode)
{
    int freg, fdir, faccess;
    const char *ptr;

    freg = fdir = faccess = 0;
    if (mode != NULL)
    {
        for (ptr = mode; *ptr != 0; ++ptr)
        {
            switch (*ptr)
            {
            case 'e':
                /* This check is assumed by default. */
                break;
            case 'f':
                freg = 1;
                break;
            case 'd':
                fdir = 1;
                break;
            case 'r':
                faccess |= OPTK_TEST_READ;
                break;
            case 'w':
                faccess |= OPTK_TEST_WRITE;
                break;
            case 'x':
                faccess |= OPTK_TEST_EXEC;
                break;
            default:
                return -1;
            }
        }
    }

#if defined OPTK_OS_WINDOWS

    {
        DWORD attr;

        attr = GetFileAttributesA(path);
        if (attr == 0xffffffffU)
            return 0;
        if (freg && (attr & FILE_ATTRIBUTE_DIRECTORY))
            return 0;
        if (fdir && !(attr & FILE_ATTRIBUTE_DIRECTORY))
            return 0;
        if ((faccess & OPTK_TEST_WRITE) && (attr & FILE_ATTRIBUTE_READONLY))
            return 0;
        return 1;
    }

#elif defined OPTK_OS_UNIX || defined OPTK_OS_DOSISH

    {
        struct stat sbuf;

        if (stat(path, &sbuf) != 0)
            return 0;
        if (freg && !(sbuf.st_mode & S_IFREG))
            return 0;
        if (fdir && !(sbuf.st_mode & S_IFDIR))
            return 0;
        if (faccess == 0)
            return 1;
        return (access(path, faccess) == 0) ? 1 : 0;
    }

#else  /* generic */

    {
        FILE *stream;

        if (fdir)
            return -1;
        if (faccess & OPTK_TEST_WRITE)
            stream = fopen(path, "r+b");
        else
            stream = fopen(path, "rb");
        if (stream == NULL)
            return 0;
        fclose(stream);
        return 1;
    }

#endif
}

/*
 * Determines if a path is an accessible directory.
 */
int
optk_test_dir(const char *path)
{
#if defined OPTK_OS_WINDOWS

    size_t len;
    char *wildname;
    HANDLE hFind;
    WIN32_FIND_DATAA wfd;

    len = strlen(path);
    if (len == 0)  /* current directory */
        return 1;

    /* Find files in (path + "\\*"). */
    if (len >= (size_t)(-3))  /* abnormally long path name */
        return 0;
    wildname = (char *)malloc(len + 3);
    if (wildname == NULL)  /* out of memory */
        return -1;
    strcpy(wildname, path);
    if (strchr(OPTK_PATH_PATHSEP_ALL_STR, wildname[len - 1]) == NULL)
        wildname[len++] = OPTK_PATH_PATHSEP;
    wildname[len++] = '*';
    wildname[len] = '\0';
    hFind = FindFirstFileA(wildname, &wfd);
    free(wildname);
    if (hFind == INVALID_HANDLE_VALUE)
        return 0;
    FindClose(hFind);
    return 1;

#elif defined OPTK_OS_UNIX || defined OPTK_OS_DOSISH

    struct stat sbuf;

    /* Avoid stat()'ing paths like "" or "[DRIVE]:",
     * because it returns incorrect results on some systems.
     */
    if (path[0] == 0)
        return 1;
#ifdef OPTK_PATH_DOS
    if (path[1] == ':' && path[2] == 0 && OPTK_PATH_IS_DRIVE_LETTER(path[0]))
        return 1;
#endif

    /* TODO (but not necessary on Unix):
     * Also avoid stat()'ing paths that end with dir separator.
     */

    /* Check if the path is a directory. */
    if (stat(path, &sbuf) == -1)
        return 0;
    return (sbuf.st_mode & S_IFDIR) ? 1 : 0;

#else  /* generic */

    OPTK_UNUSED(path);
    return -1;  /* unknown */

#endif
}

/*
 * Determines if two accessible paths are equivalent.
 */
int
optk_test_eq(const char *path1, const char *path2)
{
#if defined OPTK_OS_WINDOWS

    HANDLE hFile1, hFile2;
    BY_HANDLE_FILE_INFORMATION fileInfo1, fileInfo2;
    int result;

    hFile1 = CreateFileA(path1, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, 0, 0);
    if (hFile1 == INVALID_HANDLE_VALUE)
        return -1;
    hFile2 = CreateFileA(path2, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, 0, 0);
    if (hFile2 == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile1);
        return -1;
    }
    if (!GetFileInformationByHandle(hFile1, &fileInfo1) ||
        !GetFileInformationByHandle(hFile2, &fileInfo2))
    {
        /* Can't retrieve the file info. */
        result = -1;
    }
    else
    if (fileInfo1.nFileIndexLow == fileInfo2.nFileIndexLow &&
        fileInfo1.nFileIndexHigh == fileInfo2.nFileIndexHigh &&
        fileInfo1.dwVolumeSerialNumber == fileInfo2.dwVolumeSerialNumber)
    {
        /* The two paths have the same ID on the same volume. */
        result = 1;
    }
    else
    {
        /* The two paths have different IDs or sit on different volumes. */
        result = 0;
    }
    CloseHandle(hFile1);
    CloseHandle(hFile2);
    return result;

#elif defined OPTK_OS_UNIX || defined OPTK_OS_DOSISH

    struct stat sbuf1, sbuf2;

    if (stat(path1, &sbuf1) != 0 || stat(path2, &sbuf2) != 0)
    {
        /* Can't stat the paths. */
        return -1;
    }
    if (sbuf1.st_dev == sbuf2.st_dev && sbuf1.st_ino == sbuf2.st_ino)
    {
        /* The two paths have the same device and inode numbers. */
        /* The inode numbers are reliable only if they're not 0. */
        return (sbuf1.st_ino != 0) ? 1 : -1;
    }
    else
    {
        /* The two paths have different device or inode numbers. */
        return 0;
    }

#else  /* generic */

    OPTK_UNUSED(path1);
    OPTK_UNUSED(path2);
    return -1;  /* unknown */

#endif
}

/*
 * Removes a directory entry.
 */
int
optk_unlink(const char *path)
{
#if defined OPTK_OS_WINDOWS

    return DeleteFileA(path) ? 0 : -1;

#elif defined OPTK_OS_UNIX || defined OPTK_OS_DOSISH

    return unlink(path);

#else  /* generic */

    return remove(path);

#endif
}
