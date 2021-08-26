QT = core network

QTC_LIB_DEPENDS += utils

include(../qttest.pri)

msvc {

    LIBS *= -L$$IDE_PLUGIN_PATH
    DEFINES += Q_PLUGIN_PATH=\"\\\"$$IDE_PLUGIN_PATH\\\"\"

    CDBEXT_PATH = $$IDE_BUILD_TREE\\$$IDE_LIBRARY_BASENAME
    # replace '\' with '\\'
    DEFINES += CDBEXT_PATH=\"\\\"$$replace(CDBEXT_PATH, \\\\, \\\\)\\\"\"

}

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
DUMPERDIR   = $$IDE_SOURCE_TREE/share/qtcreator/debugger

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
    DEFINES += DEFAULT_QMAKE_BINARY=\"qmake\"
} else {
    DEFINES += DUMPERDIR=\\\"$$DUMPERDIR\\\"
    DEFINES += DEFAULT_QMAKE_BINARY=\\\"qmake\\\"
}

INCLUDEPATH += $$DEBUGGERDIR

# clang 3.5 does not like to optimize long functions.
# likewise gcc 5.4
clang|gcc: QMAKE_CXXFLAGS_RELEASE =
