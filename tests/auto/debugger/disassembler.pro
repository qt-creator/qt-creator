QT = core network widgets
include(../qttest.pri)

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger

INCLUDEPATH += $$DEBUGGERDIR

SOURCES += \
    tst_disassembler.cpp \
    $$DEBUGGERDIR/disassemblerlines.cpp \

