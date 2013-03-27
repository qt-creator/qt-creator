QT = core gui
macx:CONFIG -= app_bundle
win32:CONFIG += console
TEMPLATE = app
TARGET = cplusplus-update-frontend
DESTDIR = ./
DEFINES += QTCREATOR_UTILS_STATIC_LIB

include(../../../qtcreator.pri)
include(../../libs/cplusplus/cplusplus-lib.pri)

DEFINES *= QT_NO_CAST_FROM_ASCII
DEFINES += PATH_CPP_FRONTEND=\\\"$$PWD/../../libs/3rdparty/cplusplus\\\"
DEFINES += PATH_DUMPERS_FILE=\\\"$$PWD/../../../tests/tools/cplusplus-ast2png/dumpers.inc\\\"
SOURCES += cplusplus-update-frontend.cpp ../../libs/utils/changeset.cpp
