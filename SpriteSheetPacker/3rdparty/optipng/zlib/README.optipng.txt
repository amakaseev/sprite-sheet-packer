Name: zlib
Summary: A general-purpose data compression library
Authors: Jean-loup Gailly and Mark Adler
Version: 1.2.8
License: zlib
URL: http://zlib.net/

Modified version: 1.2.8-optipng
Modifications:
- Defined NO_GZCOMPRESS and NO_GZIP.
- Set TOO_FAR to the largest possible value, in the hope that zlib
  will produce slightly better-compressed deflate streams.
- Changed ZLIB_VERSION to "1.2.8-optipng" and ZLIB_VERNUM to 0x128f.
