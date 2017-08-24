QTC_LIB_DEPENDS += \
    clangsupport

include(../../qtcreatortool.pri)
include(../../shared/clang/clang_installation.pri)
include(source/clangpchmanagerbackend-source.pri)

requires(!isEmpty(LIBTOOLING_LIBS))

win32 {
    LLVM_BUILDMODE = $$system($$llvm_config --build-mode, lines)
    CONFIG(debug, debug|release):requires(equals(LLVM_BUILDMODE, "Debug"))
}

QT += core network
QT -= gui

LIBS += $$LIBTOOLING_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

QMAKE_CXXFLAGS += $$LLVM_CXXFLAGS

INCLUDEPATH += ../clangrefactoringbackend/source

SOURCES += \
    clangpchmanagerbackendmain.cpp \
    ../clangrefactoringbackend/source/clangtool.cpp \
    ../clangrefactoringbackend/source/refactoringcompilationdatabase.cpp


unix {
    !macx: QMAKE_LFLAGS += -Wl,-z,origin
    !disable_external_rpath: QMAKE_LFLAGS += -Wl,-rpath,$$shell_quote($${LLVM_LIBDIR})
}

DEFINES += CLANG_COMPILER_PATH=\"R\\\"xxx($$LLVM_INSTALL_DIR/bin/clang)xxx\\\"\"
