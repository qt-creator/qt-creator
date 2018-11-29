QTC_LIB_DEPENDS += \
    clangsupport

include(../../qtcreatortool.pri)
include(../../shared/clang/clang_installation.pri)
include(source/clangrefactoringbackend-source.pri)

requires(!isEmpty(LIBTOOLING_LIBS))

win32 {
    LLVM_BUILDMODE = $$system($$llvm_config --build-mode, lines)
    CONFIG(debug, debug|release):requires(equals(LLVM_BUILDMODE, "Debug"))
}

QT += core network
QT -= gui

LIBS += $$LIBTOOLING_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH
INCLUDEPATH += ../clangpchmanagerbackend/source

QMAKE_CXXFLAGS_WARN_ON *= $$LLVM_CXXFLAGS_WARNINGS
QMAKE_CXXFLAGS *= $$LLVM_CXXFLAGS

SOURCES += \
    clangrefactoringbackendmain.cpp
