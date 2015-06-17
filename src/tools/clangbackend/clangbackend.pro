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

osx {
    QMAKE_LFLAGS += -Wl,-rpath,$${LLVM_LIBDIR}
}
