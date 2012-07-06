include(../qttest.pri)

LIBS *= -L$$IDE_LIBRARY_PATH -lUtils

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
INCLUDEPATH += $$DEBUGGERDIR

SOURCES = tst_namedemangler.cpp
include($$DEBUGGERDIR/namedemangler/namedemangler.pri)

