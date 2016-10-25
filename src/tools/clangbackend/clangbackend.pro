QTC_LIB_DEPENDS += \
    sqlite \
    clangbackendipc

include(../../qtcreatortool.pri)
include(../../shared/clang/clang_installation.pri)
include(ipcsource/clangbackendclangipc-source.pri)

QT += core network
QT -= gui

LIBS += $$LIBCLANG_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

SOURCES += clangbackendmain.cpp

HEADERS += ../qtcreatorcrashhandler/crashhandlersetup.h
SOURCES += ../qtcreatorcrashhandler/crashhandlersetup.cpp

unix {
    !osx: QMAKE_LFLAGS += -Wl,-z,origin
    !contains(QMAKE_DEFAULT_LIBDIRS, $${LLVM_LIBDIR}):!disable_external_rpath: QMAKE_LFLAGS += -Wl,-rpath,$$shell_quote($${LLVM_LIBDIR})
}
