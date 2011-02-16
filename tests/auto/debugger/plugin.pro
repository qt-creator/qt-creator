include(../qttest.pri)

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
UTILSDIR    = $$IDE_SOURCE_TREE/src/libs
MACROSDIR   = $$IDE_SOURCE_TREE/share/qtcreator/gdbmacros

SOURCES += \
    tst_plugin.cpp \

DEFINES += MACROSDEBUG

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR $$MACROSDIR
