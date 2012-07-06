include(../qttest.pri)

APPSOURCEDIR = $$CREATORSOURCEDIR/src/plugins/qt4projectmanager/wizards
LIBS *= -L$$IDE_LIBRARY_PATH -lUtils

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
INCLUDEPATH += $$DEBUGGERDIR

HEADERS += \
    $$DEBUGGERDIR/namedemangler/namedemangler.h \
    $$DEBUGGERDIR/namedemangler/parsetreenodes.h
SOURCES += \
    tst_namedemangler.cpp \
    $$DEBUGGERDIR/namedemangler/namedemangler.cpp \
    $$DEBUGGERDIR/namedemangler/parsetreenodes.cpp
