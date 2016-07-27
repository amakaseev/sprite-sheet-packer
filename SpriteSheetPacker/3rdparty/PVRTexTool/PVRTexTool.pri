
INCLUDEPATH += $$PWD/Include


HEADERS += $$PWD/Include/PVRTArray.h \
    $$PWD/Include/PVRTDecompress.h \
    $$PWD/Include/PVRTError.h \
    $$PWD/Include/PVRTexture.h \
    $$PWD/Include/PVRTextureDefines.h \
    $$PWD/Include/PVRTextureFormat.h \
    $$PWD/Include/PVRTextureHeader.h \
    $$PWD/Include/PVRTextureUtilities.h \
    $$PWD/Include/PVRTextureVersion.h \
    $$PWD/Include/PVRTGlobal.h \
    $$PWD/Include/PVRTMap.h \
    $$PWD/Include/PVRTString.h \
    $$PWD/Include/PVRTTexture.h

macx {
    LIBS += $$PWD/OSX_x86/libPVRTexLib.dylib

    export_dylib.files = $$PWD/OSX_x86/libPVRTexLib.dylib
    export_dylib.path = Contents/lib
    QMAKE_BUNDLE_DATA += export_dylib
}

win32 {
    DEFINES += _WINDLL_IMPORT
    LIBS += $$PWD/Windows_x86_64/PVRTexLib.lib
}
#TODO: test on windows platform
#win32:contains(QMAKE_HOST.arch, x86_64) {
#    LIBS += $$PWD/Windows_x86_64/PVRTexLib.lib
#    export_dll.files = $$PWD/Windows_x86_64/PVRTexLib.dll
#    export_dll.path = ./
#    DEPLOYMENT += export_dll
#}
#win32:contains(QMAKE_HOST.arch, x86_32) {
#    LIBS += $$PWD/Windows_x86_32/PVRTexLib.lib
#    export_dll.files = $$PWD/Windows_x86_32/PVRTexLib.dll
#    export_dll.path = ./
#    DEPLOYMENT += export_dll
#}
