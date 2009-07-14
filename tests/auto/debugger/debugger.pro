QT = core testlib gui


DEBUGGERDIR = ../../../src/plugins/debugger
UTILSDIR    = ../../../src/libs
MACROSDIR   = ../../../share/qtcreator/gdbmacros

SOURCES += \
    $$DEBUGGERDIR/gdb/gdbmi.cpp \
    $$DEBUGGERDIR/tcf/json.cpp \
    $$MACROSDIR/gdbmacros.cpp \
    main.cpp \

DEFINES += MACROSDEBUG

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR $$MACROSDIR

