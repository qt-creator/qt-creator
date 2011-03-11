include(valgrindtoolbase_dependencies.pri)

INCLUDEPATH += $$PWD
LIBS *= -l$$qtLibraryName(ValgrindToolBase)
