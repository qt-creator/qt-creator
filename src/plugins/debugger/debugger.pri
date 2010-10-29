include(debugger_dependencies.pri)

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS *= -l$$qtLibraryName(Debugger)
