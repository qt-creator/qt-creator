include($$replace(_PRO_FILE_PWD_, ([^/]+$), \\1/\\1_dependencies.pri))
TARGET = $$QTC_LIB_NAME

include(../qtcreator.pri)

# use precompiled header for libraries by default
isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/shared/qtcreator_pch.h

win32 {
    DLLDESTDIR = $$IDE_APP_PATH
}

DESTDIR = $$IDE_LIBRARY_PATH

osx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
    QMAKE_LFLAGS += -compatibility_version $$QTCREATOR_COMPAT_VERSION
}

RPATH_BASE = $$IDE_LIBRARY_PATH
include(rpath.pri)

TARGET = $$qtLibraryTargetName($$TARGET)

TEMPLATE = lib
CONFIG += shared dll

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

win32 {
    dlltarget.path = $$INSTALL_BIN_PATH
    INSTALLS += dlltarget
} else {
    target.path = $$INSTALL_LIBRARY_PATH
    INSTALLS += target
}
