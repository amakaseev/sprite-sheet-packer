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
QMAKE_CXXFLAGS +=-std=c++11 -stdlib=libc++

ICON = SpritePacker.icns
RC_FILE = SpritePacker.rc

INCLUDEPATH = ./ ../shared

SOURCES += main.cpp\
    mainwindow.cpp \
    ../shared/spriteatlas.cpp

HEADERS += mainwindow.h \
    ../shared/imagerotate.h \
    ../shared/binpack2d.hpp \
    ../shared/spriteatlas.h

FORMS += mainwindow.ui

RESOURCES += \
    resources.qrc

include(../shared/qtplist-master/qtplist-master.pri)
