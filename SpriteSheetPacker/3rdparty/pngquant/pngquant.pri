QMAKE_CFLAGS += -std=c11

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/blur.h \
    $$PWD/libimagequant.h \
    $$PWD/mediancut.h \
    $$PWD/mempool.h \
    $$PWD/nearest.h \
    $$PWD/pam.h \
    $$PWD/rwpng.h \
	$$PWD/viter.h

SOURCES += \
    $$PWD/blur.c \
    $$PWD/libimagequant.c \
    #$$PWD/pngquant.c \
    $$PWD/mediancut.c \
    $$PWD/mempool.c \
    $$PWD/nearest.c \
    $$PWD/pam.c \
    $$PWD/rwpng.c \
	$$PWD/viter.c
