QT += testlib

#QT -= gui

UTILSDIR    = ../../../src/libs

DEBUGGERDIR = ../../../src/plugins/debugger

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR

SOURCES += \
    tst_gdb.cpp \
    $$DEBUGGERDIR/gdb/gdbmi.cpp \

TARGET = tst_$$TARGET

