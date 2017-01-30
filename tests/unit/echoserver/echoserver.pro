QT       += core network

TARGET = echo
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

unix:LIBS += -ldl

# Set IDE_LIBEXEC_PATH and IDE_BIN_PATH to silence a warning about empty
# QTC_REL_TOOLS_PATH, which is not used by the tests.
IDE_LIBEXEC_PATH=$$PWD
IDE_BIN_PATH=$$PWD
include($$PWD/../../../src/libs/utils/utils-lib.pri)

include(../../../qtcreator.pri)
include(../../../src/libs/clangbackendipc/clangbackendipc-lib.pri)
include(../../../src/libs/sqlite/sqlite-lib.pri)

INCLUDEPATH += ../../../src/libs

SOURCES += \
    echoclangcodemodelserver.cpp \
    echoserverprocessmain.cpp

HEADERS += \
    echoclangcodemodelserver.h

DEFINES += CLANGBACKENDIPC_TESTS
DEFINES += DONT_CHECK_MESSAGE_COUNTER

win32:DESTDIR = ..
