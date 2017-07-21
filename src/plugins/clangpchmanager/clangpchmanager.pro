include(../../qtcreatorplugin.pri)
include(clangpchmanager-source.pri)
include(../../shared/clang/clang_installation.pri)
include(../../shared/clang/clang_defines.pri)

requires(!isEmpty(LIBTOOLING_LIBS))

HEADERS += \
    $$PWD/clangpchmanagerplugin.h \
    qtcreatorprojectupdater.h
SOURCES += \
    $$PWD/clangpchmanagerplugin.cpp \
    qtcreatorprojectupdater.cpp
