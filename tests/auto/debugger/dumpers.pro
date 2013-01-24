
include(../qttest.pri)

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
DUMPERDIR   = $$IDE_SOURCE_TREE/share/qtcreator/dumper

SOURCES += \
    $$DEBUGGERDIR/debuggerprotocol.cpp \
    $$DEBUGGERDIR/watchdata.cpp \
    $$DEBUGGERDIR/watchutils.cpp \
    tst_dumpers.cpp

HEADERS += \
    $$DEBUGGERDIR/debuggerprotocol.h \
    $$DEBUGGERDIR/watchdata.h \
    $$DEBUGGERDIR/watchutils.h \

!isEmpty(vcproj) {
    DEFINES += DUMPERDIR=\"$$DUMPERDIR\"
} else {
    DEFINES += DUMPERDIR=\\\"$$DUMPERDIR\\\"
}

INCLUDEPATH += $$DEBUGGERDIR
DEFINES += QT_NO_CAST_FROM_ASCII
