QTC_LIB_DEPENDS += \
    sqlite \
    codemodelbackendipc

QT += core network
QT -= gui

TARGET = codemodelbackend
CONFIG += console
CONFIG -= app_bundle C++-14

TEMPLATE = app

include(ipcsource/codemodelbackendclangipc-source.pri)
include(../../../qtcreator.pri)
include(../../shared/clang/clang_installation.pri)
include(../../rpath.pri)

LIBS += $$LLVM_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

SOURCES += codemodelbackendmain.cpp

osx {
    QMAKE_LFLAGS += -Wl,-rpath,$${LLVM_LIBDIR}
}
