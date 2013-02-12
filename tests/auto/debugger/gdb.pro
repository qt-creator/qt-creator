

# These tests should be merged into tst_dumpers.cpp

include(../qttest.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)

UTILSDIR    = $$IDE_SOURCE_TREE/src/libs

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR

SOURCES += \
    tst_gdb.cpp \
    $$DEBUGGERDIR/debuggerprotocol.cpp \
