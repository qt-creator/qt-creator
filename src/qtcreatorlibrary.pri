include(../qtcreator.pri)

include($$replace(_PRO_FILE_PWD_, ([^/]+$), \\1/\\1_dependencies.pri))

# use precompiled header for libraries by default
isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/shared/qtcreator_pch.h

win32 {
    DLLDESTDIR = $$IDE_APP_PATH
}

DESTDIR = $$IDE_LIBRARY_PATH

include(rpath.pri)

TARGET = $$qtLibraryName($$TARGET)

CONFIG += shared dll

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

!macx {
    win32 {
        dlltarget.path = $$QTC_PREFIX/bin
        INSTALLS += dlltarget
    } else {
        target.path = $$QTC_PREFIX/$$IDE_LIBRARY_BASENAME/qtcreator
        INSTALLS += target
    }
}
