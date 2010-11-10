INCLUDEPATH += $$PWD/../../shared
INCLUDEPATH += $$PWD/../../shared/glsl $$PWD/../../shared/glsl/parser

DEPENDPATH += $$PWD/../../shared/glsl $$PWD/../../shared/glsl/parser
LIBS *= -l$$qtLibraryName(GLSL)
DEFINES += QT_CREATOR
