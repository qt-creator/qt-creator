
QT = core testlib

DEBUGGERDIR = ../../../src/plugins/debugger
UTILSDIR = ../../../src/libs

SOURCES += \
    $$DEBUGGERDIR/gdbmi.cpp \
    main.cpp \

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR

