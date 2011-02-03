include(../qttest.pri)

DEBUGGERDIR = ../../../src/plugins/debugger
UTILSDIR    = ../../../src/libs
MACROSDIR   = ../../../share/qtcreator/gdbmacros

SOURCES += \
    $$DEBUGGERDIR/gdb/gdbmi.cpp \
    $$DEBUGGERDIR/tcf/json.cpp \
    $$MACROSDIR/gdbmacros.cpp \
    tst_dumpers.cpp \

DEFINES += MACROSDEBUG
win32:DEFINES += _CRT_SECURE_NO_WARNINGS

DEFINES -= QT_USE_FAST_CONCATENATION QT_USE_FAST_OPERATOR_PLUS
DEFINES -= QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR $$MACROSDIR
