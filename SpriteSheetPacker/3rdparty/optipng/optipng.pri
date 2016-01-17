
INCLUDEPATH += $$PWD

#optipng
INCLUDEPATH += $$PWD/optipng
HEADERS += \
    $$PWD/optipng/proginfo.h \
    $$PWD/optipng/strconv.h
SOURCES += $$PWD/optipng/strconv.c

#opngreduc
INCLUDEPATH += $$PWD/opngreduc
HEADERS += \
    $$PWD/opngreduc/opngreduc.h
SOURCES += \
    $$PWD/opngreduc/opngreduc.c

#libpng
INCLUDEPATH += $$PWD/libpng
HEADERS += \
    $$PWD/libpng/pnglibconf.h \
    $$PWD/libpng/png.h \
    $$PWD/libpng/pngconf.h \
    $$PWD/libpng/pnglibconf.h \
    $$PWD/libpng/pngdebug.h \
    $$PWD/libpng/pnginfo.h \
    $$PWD/libpng/pngpriv.h \
    $$PWD/libpng/pngstruct.h \
    $$PWD/libpng/pngusr.h
SOURCES += \
    $$PWD/libpng/png.c \
    $$PWD/libpng/pngerror.c \
    $$PWD/libpng/pngget.c \
    $$PWD/libpng/pngmem.c \
    $$PWD/libpng/pngpread.c \
    $$PWD/libpng/pngread.c \
    $$PWD/libpng/pngrio.c \
    $$PWD/libpng/pngrtran.c \
    $$PWD/libpng/pngrutil.c \
    $$PWD/libpng/pngset.c \
    $$PWD/libpng/pngtrans.c \
    $$PWD/libpng/pngwio.c \
    $$PWD/libpng/pngwrite.c \
    $$PWD/libpng/pngwtran.c \
    $$PWD/libpng/pngwutil.c

#pngxtern
INCLUDEPATH += $$PWD/pngxtern
HEADERS += \
    $$PWD/pngxtern/pngxpriv.h \
    $$PWD/pngxtern/pngxtern.h \
    $$PWD/pngxtern/pngxutil.h
SOURCES += \
    $$PWD/pngxtern/pngxmem.c \
    $$PWD/pngxtern/pngxrbmp.c \
    $$PWD/pngxtern/pngxread.c \
    $$PWD/pngxtern/pngxrgif.c \
    $$PWD/pngxtern/pngxrjpg.c \
    $$PWD/pngxtern/pngxrpnm.c \
    $$PWD/pngxtern/pngxrtif.c \
    $$PWD/pngxtern/pngxset.c

#cexcept
INCLUDEPATH += $$PWD/cexcept
HEADERS += $$PWD/cexcept/cexcept.h

#cexcept
INCLUDEPATH += $$PWD/gifread
HEADERS += $$PWD/gifread/gifread.h
SOURCES += $$PWD/gifread/gifread.c

#pnmio
INCLUDEPATH += $$PWD/pnmio
HEADERS += $$PWD/pnmio/pnmio.h
SOURCES += \
    $$PWD/pnmio/pnmin.c \
    $$PWD/pnmio/pnmout.c \
    $$PWD/pnmio/pnmutil.c

#minitiff
INCLUDEPATH += $$PWD/minitiff
HEADERS += \
    $$PWD/minitiff/minitiff.h \
    $$PWD/minitiff/tiffdef.h
SOURCES += \
    $$PWD/minitiff/tiffbase.c \
    $$PWD/minitiff/tiffread.c \
    $$PWD/minitiff/tiffwrite.c

#opnglib
INCLUDEPATH += \
	$$PWD/opnglib \
	$$PWD/opnglib/include/opnglib \
	$$PWD/opnglib/include/optk \
	$$PWD/opnglib/src/opngcore \
	$$PWD/opnglib/src/opngtrans
	
HEADERS += \
    $$PWD/opnglib/include/opnglib/opngcore.h \
	$$PWD/opnglib/include/opnglib/opnglib.h \
	$$PWD/opnglib/include/opnglib/opngtrans.h \
	$$PWD/opnglib/include/opnglib/optk.h \
	$$PWD/opnglib/include/optk/bits.h \
	$$PWD/opnglib/include/optk/integer.h \
	$$PWD/opnglib/include/optk/io.h \
	$$PWD/opnglib/src/opngcore/codec.h \
	$$PWD/opnglib/src/opngcore/image.h \
	$$PWD/opnglib/src/opngcore/ioenv.h \
	$$PWD/opnglib/src/opngcore/util.h \
	$$PWD/opnglib/src/opngcore/version.h \
	$$PWD/opnglib/src/opngtrans/chunksig.h \
	$$PWD/opnglib/src/opngtrans/parser.h \
	$$PWD/opnglib/src/opngtrans/trans.h


SOURCES += \
    $$PWD/opnglib/src/opngcore/codec.c \
	$$PWD/opnglib/src/opngcore/image.c \
	$$PWD/opnglib/src/opngcore/ioenv.c \
	$$PWD/opnglib/src/opngcore/logger.c \
	$$PWD/opnglib/src/opngcore/optim.c \
	$$PWD/opnglib/src/opngcore/util.c \
	$$PWD/opnglib/src/opngcore/version.c \
	$$PWD/opnglib/src/opngtrans/apply.c \
	$$PWD/opnglib/src/opngtrans/chunksig.c \
	$$PWD/opnglib/src/opngtrans/parser.c \
	$$PWD/opnglib/src/opngtrans/trans.c \
	$$PWD/opnglib/src/optk/bits.c \
	$$PWD/opnglib/src/optk/io.c \
	$$PWD/opnglib/src/optk/wildargs.c \
	
#sysexits
INCLUDEPATH += $$PWD/sysexits
HEADERS += $$PWD/sysexits/sysexits.h
	
#zlib
INCLUDEPATH += $$PWD/zlib
HEADERS += \
    $$PWD/zlib/crc32.h \
    $$PWD/zlib/deflate.h \
    $$PWD/zlib/gzguts.h \
    $$PWD/zlib/inffast.h \
    $$PWD/zlib/inffixed.h \
    $$PWD/zlib/inflate.h \
    $$PWD/zlib/inftrees.h \
    $$PWD/zlib/trees.h \
    $$PWD/zlib/zconf.h \
    $$PWD/zlib/zlib.h \
    $$PWD/zlib/zutil.h
SOURCES += \
    $$PWD/zlib/adler32.c \
    $$PWD/zlib/compress.c \
    $$PWD/zlib/crc32.c \
    $$PWD/zlib/deflate.c \
    $$PWD/zlib/gzclose.c \
    $$PWD/zlib/gzlib.c \
    $$PWD/zlib/gzread.c \
    $$PWD/zlib/gzwrite.c \
    $$PWD/zlib/infback.c \
    $$PWD/zlib/inffast.c \
    $$PWD/zlib/inflate.c \
    $$PWD/zlib/inftrees.c \
    $$PWD/zlib/trees.c \
    $$PWD/zlib/uncompr.c \
    $$PWD/zlib/zutil.c
