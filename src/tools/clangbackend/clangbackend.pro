QTC_LIB_DEPENDS += \
    sqlite \
    clangbackendipc

include(../../qtcreatortool.pri)
include(../../shared/clang/clang_installation.pri)
include(ipcsource/clangbackendclangipc-source.pri)

QT += core network
QT -= gui

LIBS += $$LLVM_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

SOURCES += clangbackendmain.cpp

unix {
    !osx: QMAKE_LFLAGS += -Wl,-z,origin
    QMAKE_LFLAGS += -Wl,-rpath,$$shell_quote($${LLVM_LIBDIR})
}
