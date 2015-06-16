QTC_LIB_DEPENDS += \
    sqlite \
    clangbackendipc

QT += core network
QT -= gui

TARGET = clangbackend
CONFIG += console
CONFIG -= app_bundle C++-14

TEMPLATE = app

include(ipcsource/clangbackendclangipc-source.pri)
include(../../../qtcreator.pri)
include(../../shared/clang/clang_installation.pri)
include(../../rpath.pri)

LIBS += $$LLVM_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

SOURCES += clangbackendmain.cpp

osx {
    QMAKE_LFLAGS += -Wl,-rpath,$${LLVM_LIBDIR}
}
