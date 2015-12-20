#-------------------------------------------------
#
# Project created by QtCreator 2014-01-04T16:27:31
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SpritePacker
TEMPLATE = app

CONFIG += c++11
#QMAKE_CXXFLAGS +=-std=c++11 -stdlib=libc++

ICON = SpritePacker.icns
RC_FILE = SpritePacker.rc

SOURCES += main.cpp\
    MainWindow.cpp \
    SpriteAtlas.cpp \
    ScalingVariantWidget.cpp \
    SpritePackerProjectFile.cpp

HEADERS += MainWindow.h \
    ImageRotate.h \
    SpriteAtlas.h \
    binpack2d.hpp \
    ScalingVariantWidget.h \
    SpritePackerProjectFile.h

FORMS += MainWindow.ui \
    ScalingVariantWidget.ui

RESOURCES += \
    resources.qrc

include(qtplist-master/qtplist-master.pri)
