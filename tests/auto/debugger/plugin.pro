QT += testlib


DEBUGGERDIR = ../../../src/plugins/debugger
UTILSDIR    = ../../../src/libs
MACROSDIR   = ../../../share/qtcreator/gdbmacros

SOURCES += \
    tst_plugin.cpp \

DEFINES += MACROSDEBUG

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR $$MACROSDIR

TARGET = tst_$$TARGET

