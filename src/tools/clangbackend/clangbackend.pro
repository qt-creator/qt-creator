QTC_LIB_DEPENDS += \
    sqlite \
    clangsupport

include(../../qtcreatortool.pri)
include(../../shared/clang/clang_installation.pri)
include(source/clangbackendclangipc-source.pri)

requires(!isEmpty(LLVM_VERSION))

QT += core network
QT -= gui

LIBS += $$LIBCLANG_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

SOURCES += clangbackendmain.cpp

HEADERS += ../qtcreatorcrashhandler/crashhandlersetup.h
SOURCES += ../qtcreatorcrashhandler/crashhandlersetup.cpp

unix:!disable_external_rpath:!contains(QMAKE_DEFAULT_LIBDIRS, $${LLVM_LIBDIR}) {
    !osx: QMAKE_LFLAGS += -Wl,-z,origin
    QMAKE_LFLAGS += -Wl,-rpath,$$shell_quote($${LLVM_LIBDIR})
}
