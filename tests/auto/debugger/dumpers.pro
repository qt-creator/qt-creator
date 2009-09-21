QT += testlib

DEBUGGERDIR = ../../../src/plugins/debugger
UTILSDIR    = ../../../src/libs
MACROSDIR   = ../../../share/qtcreator/gdbmacros

SOURCES += \
    $$DEBUGGERDIR/gdb/gdbmi.cpp \
    $$DEBUGGERDIR/tcf/json.cpp \
    $$MACROSDIR/gdbmacros.cpp \
    tst_dumpers.cpp \

DEFINES += MACROSDEBUG

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR $$MACROSDIR

TARGET = tst_$$TARGET

