QTC_LIB_DEPENDS += \
    clangbackendipc

include(../../qtcreatortool.pri)
include(../../shared/clang/clang_installation.pri)
include(source/clangrefactoringbackend-source.pri)

win32 {
    LLVM_BUILDMODE = $$system($$llvm_config --build-mode, lines)
    CONFIG(debug, debug|release):requires(equals(LLVM_BUILDMODE, "Debug"))
}

QT += core network
QT -= gui

LIBS += $$LIBTOOLING_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

QMAKE_CXXFLAGS += $$LLVM_CXXFLAGS

SOURCES += \
    clangrefactoringbackendmain.cpp

unix {
    !osx: QMAKE_LFLAGS += -Wl,-z,origin
    !disable_external_rpath: QMAKE_LFLAGS += -Wl,-rpath,$$shell_quote($${LLVM_LIBDIR})
}
