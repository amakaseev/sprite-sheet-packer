#-------------------------------------------------
#
# Project created by QtCreator 2014-01-04T16:27:31
#
#-------------------------------------------------

QT += core widgets xml qml

TARGET = SpriteSheetPacker
TEMPLATE = app

CONFIG += c++11
#QMAKE_CXXFLAGS +=-std=c++11 -stdlib=libc++

SOURCES += main.cpp\
    MainWindow.cpp \
    SpriteAtlas.cpp \
    ScalingVariantWidget.cpp \
    SpritePackerProjectFile.cpp \
    PublishSpriteSheet.cpp \
    PreferencesDialog.cpp \
    PublishStatusDialog.cpp

HEADERS += MainWindow.h \
    ImageRotate.h \
    SpriteAtlas.h \
    binpack2d.hpp \
    ScalingVariantWidget.h \
    SpritePackerProjectFile.h \
    PublishSpriteSheet.h \
    PreferencesDialog.h \
    PublishStatusDialog.h

FORMS += MainWindow.ui \
    ScalingVariantWidget.ui \
    PreferencesDialog.ui \
    PublishStatusDialog.ui

RESOURCES += resources.qrc

include(qtplist-master/qtplist-master.pri)

OTHER_FILES += \
    defaultFormats/cocos2d.js \
    defaultFormats/json.js \

macx {
    ICON = SpritePacker.icns
    exportData.files += defaultFormats/cocos2d.js defaultFormats/cocos2d.png
    exportData.files += defaultFormats/json.js defaultFormats/json.png
    exportData.path = Contents/MacOS/defaultFormats
    QMAKE_BUNDLE_DATA += exportData
}

win32 {
    RC_ICONS = SpritePacker.ico
    exportData.files = defaultFormats/*.*
    exportData.path = defaultFormats
    DEPLOYMENT += exportData
}


isEmpty(TARGET_EXT) {
    win32 {
        TARGET_CUSTOM_EXT = .exe
    }
    macx {
        TARGET_CUSTOM_EXT = .app
    }
} else {
    TARGET_CUSTOM_EXT = $${TARGET_EXT}
}

win32 {
    DEPLOY_COMMAND = windeployqt
}
macx {
    DEPLOY_COMMAND = macdeployqt
}

CONFIG(release,debug|release) {
    # release
    DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/$${TARGET}$${TARGET_CUSTOM_EXT}))

    warning($${DEPLOY_COMMAND} $${DEPLOY_TARGET})
    QMAKE_POST_LINK = $${DEPLOY_COMMAND} $${DEPLOY_TARGET}
}
