include(../../qtcreatorplugin.pri)
include(clangrefactoring-source.pri)
include(../../shared/clang/clang_installation.pri)

DEFINES += CLANG_VERSION=\\\"$${LLVM_VERSION}\\\"
DEFINES += "\"CLANG_RESOURCE_DIR=\\\"$${LLVM_LIBDIR}/clang/$${LLVM_VERSION}/include\\\"\""

HEADERS += \
    $$PWD/clangrefactoringplugin.h

SOURCES += \
    $$PWD/clangrefactoringplugin.cpp
