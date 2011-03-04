include(valgrindtoolbase_dependencies.pri)

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS *= -l$$qtLibraryName(ValgrindToolBase)
