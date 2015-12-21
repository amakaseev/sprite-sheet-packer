#-------------------------------------------------
#
# Project created by QtCreator 2014-01-04T16:27:31
#
#-------------------------------------------------

QT += core widgets xml qml

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
    SpritePackerProjectFile.cpp \
    PublishSpriteSheet.cpp

HEADERS += MainWindow.h \
    ImageRotate.h \
    SpriteAtlas.h \
    binpack2d.hpp \
    ScalingVariantWidget.h \
    SpritePackerProjectFile.h \
    PublishSpriteSheet.h

FORMS += MainWindow.ui \
    ScalingVariantWidget.ui

RESOURCES += resources.qrc

include(qtplist-master/qtplist-master.pri)

OTHER_FILES += \
    defaultPublish/cocos2d-x.js \
    defaultPublish/json.js \

macx {
    exportData.files = defaultPublish/cocos2d-x.js defaultPublish/json.js
    exportData.path = Contents/MacOS/defaultPublish
    QMAKE_BUNDLE_DATA += exportData
}

win32 {
    exportData.files = defaultPublish/*.*
    exportData.path = defaultPublish
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
