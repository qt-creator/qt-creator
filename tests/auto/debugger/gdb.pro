include(../qttest.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger

INCLUDEPATH += $$DEBUGGERDIR

SOURCES += \
    tst_gdb.cpp \
    $$DEBUGGERDIR/debuggerprotocol.cpp \

