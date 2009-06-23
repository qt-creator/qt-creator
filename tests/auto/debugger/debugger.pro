
QT = core testlib

DEBUGGERDIR = ../../../src/plugins/debugger
UTILSDIR = ../../../src/libs

SOURCES += \
    $$DEBUGGERDIR/gdb/gdbmi.cpp \
    $$DEBUGGERDIR/tcf/json.cpp \
    main.cpp \

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR

