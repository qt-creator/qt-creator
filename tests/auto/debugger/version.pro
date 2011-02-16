include(../qttest.pri)

UTILSDIR    = $$IDE_SOURCE_TREE/src/libs
DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR

SOURCES += \
    tst_version.cpp \
    $$DEBUGGERDIR/gdb/gdbmi.cpp \

