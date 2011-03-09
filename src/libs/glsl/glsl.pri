INCLUDEPATH += \
    $$PWD/../../shared \
    $$PWD/../../shared/glsl \
    $$PWD/../../shared/glsl/parser

LIBS *= -l$$qtLibraryName(GLSL)
DEFINES += QT_CREATOR
