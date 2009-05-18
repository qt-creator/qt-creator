
QT = core testlib

DEBUGGERDIR = ../../../src/plugins/debugger
UTILSDIR = ../../../src/libs

SOURCES += \
    $$DEBUGGERDIR/gdbmi.cpp \
    $$DEBUGGERDIR/json.cpp \
    main.cpp \

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR

