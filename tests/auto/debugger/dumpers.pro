QT = core network

win32-msvc* {
    QTC_LIB_DEPENDS += utils
}

include(../qttest.pri)

win32-msvc* {
    LIBS *= -L$$IDE_PLUGIN_PATH
    DEFINES += Q_PLUGIN_PATH=\"\\\"$$IDE_PLUGIN_PATH\\\"\"

    CDBEXT_PATH = $$IDE_BUILD_TREE\\$$IDE_LIBRARY_BASENAME
    # replace '\' with '\\'
    DEFINES += CDBEXT_PATH=\"\\\"$$replace(CDBEXT_PATH, \\\\, \\\\)\\\"\"
}

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
DUMPERDIR   = $$IDE_SOURCE_TREE/share/qtcreator/debugger

include($$IDE_SOURCE_TREE/src/rpath.pri)


SOURCES += \
    $$DEBUGGERDIR/debuggerprotocol.cpp \
    $$DEBUGGERDIR/simplifytype.cpp \
    $$DEBUGGERDIR/watchdata.cpp \
    $$DEBUGGERDIR/watchutils.cpp \
    tst_dumpers.cpp

HEADERS += \
    $$DEBUGGERDIR/debuggerprotocol.h \
    $$DEBUGGERDIR/simplifytype.h \
    $$DEBUGGERDIR/watchdata.h \
    $$DEBUGGERDIR/watchutils.h

!isEmpty(vcproj) {
    DEFINES += DUMPERDIR=\"$$DUMPERDIR\"
} else {
    DEFINES += DUMPERDIR=\\\"$$DUMPERDIR\\\"
}

INCLUDEPATH += $$DEBUGGERDIR
DEFINES += QT_NO_CAST_FROM_ASCII

# clang 3.5 does not like to optimize long functions.
clang: QMAKE_CXXFLAGS_RELEASE =
