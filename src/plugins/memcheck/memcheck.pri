include(memcheck_dependencies.pri)

INCLUDEPATH += $$PWD
LIBS *= -l$$qtLibraryName(Memcheck)
