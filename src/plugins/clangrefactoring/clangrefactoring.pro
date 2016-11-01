include(../../qtcreatorplugin.pri)
include(clangrefactoring-source.pri)
include(../../shared/clang/clang_installation.pri)

include(../../shared/clang/clang_defines.pri)

HEADERS += \
    $$PWD/clangrefactoringplugin.h

SOURCES += \
    $$PWD/clangrefactoringplugin.cpp
