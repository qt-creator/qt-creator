# Build the Qt Creator CDB extension
TEMPLATE = lib

include(../../../qtcreator.pri)
include(cdb_detect.pri)

CONFIG -= precompile_header
CONFIG += hide_symbols

# Switch to statically linked CRT. Note: There will be only one
# global state of the CRT, reconsider if other DLLs are required!
# TODO: No effect, currently?
QMAKE_CXXFLAGS_RELEASE    -= -MD
QMAKE_CXXFLAGS_DEBUG      -= -MDd
QMAKE_CXXFLAGS_RELEASE    += -MT
QMAKE_CXXFLAGS_DEBUG      += -MTd

BASENAME=qtcreatorcdbext

DEF_FILE=$$PWD/qtcreatorcdbext.def

IDE_BASE_PATH=$$dirname(IDE_APP_PATH)

# Find out 64/32bit and determine target directories accordingly.
# TODO: This is an ugly hack. Better check compiler (stderr) or something?
ENV_LIB_PATH=$$(LIBPATH)


contains(ENV_LIB_PATH, ^.*amd64.*$) {
    DESTDIR=$$IDE_BASE_PATH/lib/$${BASENAME}64
    CDB_PLATFORM=amd64
} else {
    DESTDIR=$$IDE_BASE_PATH/lib/$${BASENAME}32
    CDB_PLATFORM=i386
}

TARGET = $$BASENAME

message("Compiling Qt Creator CDB extension $$TARGET $$DESTDIR for $$CDB_PLATFORM using $$CDB_PATH")

INCLUDEPATH += $$CDB_PATH/inc
LIBS+= -L$$CDB_PATH/lib/$$CDB_PLATFORM -ldbgeng

CONFIG -= qt
QT -= gui
QT -= core

SOURCES += qtcreatorcdbextension.cpp \
    extensioncontext.cpp \
    eventcallback.cpp \
    symbolgroupnode.cpp \
    symbolgroup.cpp \
    common.cpp \
    stringutils.cpp \
    gdbmihelpers.cpp \
    outputcallback.cpp \
    base64.cpp \
    symbolgroupvalue.cpp \
    containers.cpp

HEADERS += extensioncontext.h \
    common.h \
    iinterfacepointer.h \
    eventcallback.h \
    symbolgroup.h \
    stringutils.h \
    gdbmihelpers.h \
    outputcallback.h \
    base64.h \
    symbolgroupvalue.h \
    containers.h \
    knowntype.h \
    symbolgroupnode.h
