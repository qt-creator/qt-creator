include(../clangtoolstest.pri)
include(../../../../src/shared/clang/clang_installation.pri)

requires(!isEmpty(LLVM_VERSION))

TARGET = tst_clangtoolslogfilereader

DEFINES += SRCDIR=\\\"$$PWD/\\\"

LIBS += $$LIBCLANG_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

SOURCES += \
    tst_clangtoolslogfilereader.cpp \
    $$PLUGINDIR/clangtoolsdiagnostic.cpp \
    $$PLUGINDIR/clangtoolslogfilereader.cpp

HEADERS += \
    $$PLUGINDIR/clangtoolsdiagnostic.h \
    $$PLUGINDIR/clangtoolslogfilereader.h
