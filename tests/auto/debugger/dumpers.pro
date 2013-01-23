
include(../qttest.pri)

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
DUMPERDIR   = $$IDE_SOURCE_TREE/share/qtcreator/dumper

SOURCES += \
    #$$DUMPERDIR/dumper.cpp \
    $$DEBUGGERDIR/debuggerprotocol.cpp \
    tst_dumpers.cpp

!isEmpty(vcproj) {
    DEFINES += DUMPERDIR=\"$$DUMPERDIR\"
} else {
    DEFINES += DUMPERDIR=\\\"$$DUMPERDIR\\\"
}

INCLUDEPATH += $$DEBUGGERDIR
DEFINES += QT_NO_CAST_FROM_ASCII
