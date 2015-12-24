QT += core
QT -= gui

TARGET = ssp
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp \
    ../SpritePacker/SpriteAtlas.cpp

HEADERS += \
    ../SpritePacker/SpriteAtlas.h

