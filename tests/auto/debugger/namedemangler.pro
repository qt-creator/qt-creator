include(../qttest.pri)

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
INCLUDEPATH += $$DEBUGGERDIR

HEADERS += $$DEBUGGERDIR/name_demangler.h
SOURCES += tst_namedemangler.cpp $$DEBUGGERDIR/name_demangler.cpp
