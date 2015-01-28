QTC_LIB_DEPENDS += utils

include(../../../qtcreator.pri)
include(../../libs/cplusplus/cplusplus-lib.pri)

DESTDIR = $$IDE_BIN_PATH

include(../../rpath.pri)

QT = core gui
macx:CONFIG -= app_bundle
win32:CONFIG += console
TEMPLATE = app
TARGET = cplusplus-update-frontend

DEFINES += PATH_CPP_FRONTEND=\\\"$$PWD/../../libs/3rdparty/cplusplus\\\"
DEFINES += PATH_DUMPERS_FILE=\\\"$$PWD/../../../tests/tools/cplusplus-ast2png/dumpers.inc\\\"
SOURCES += cplusplus-update-frontend.cpp
