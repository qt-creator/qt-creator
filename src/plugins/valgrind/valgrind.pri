include(valgrind_dependencies.pri)

INCLUDEPATH += $$PWD
LIBS *= -l$$qtLibraryName(Valgrind)
