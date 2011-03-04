include(memcheck_dependencies.pri)

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS *= -l$$qtLibraryName(Memcheck)
