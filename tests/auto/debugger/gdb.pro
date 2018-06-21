QT = core network
include(../qttest.pri)

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
UTILSDIR = $$IDE_SOURCE_TREE/src/libs/utils

INCLUDEPATH += $$DEBUGGERDIR

SOURCES += \
    tst_gdb.cpp \
    $$DEBUGGERDIR/debuggerprotocol.cpp \
    $$UTILSDIR/processhandle.cpp

