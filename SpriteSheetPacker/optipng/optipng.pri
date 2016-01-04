
INCLUDEPATH += $$PWD
LIBS += -lz
#optipng
INCLUDEPATH += $$PWD/optipng
HEADERS += \
    $$PWD/optipng/optipng.h \
    $$PWD/optipng/bitset.h \
    $$PWD/optipng/proginfo.h \
    $$PWD/optipng/osys.h \
    $$PWD/optipng/ratio.h
SOURCES += \
    $$PWD/optipng/optim.c \
    $$PWD/optipng/bitset.c \
    $$PWD/optipng/osys.c \
    $$PWD/optipng/ratio.c

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
    $$PWD/libpng/pngstruct.h
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
    $$PWD/pngxtern/pngxio.c \
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
