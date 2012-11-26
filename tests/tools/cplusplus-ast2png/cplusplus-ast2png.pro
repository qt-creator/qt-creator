QT = core gui
macx:CONFIG -= app_bundle
win32:CONFIG += console
TEMPLATE = app
TARGET = cplusplus-ast2png
DESTDIR = ./

include(../../../qtcreator.pri)
include(../../../src/libs/cplusplus/cplusplus-lib.pri)
include(../../../src/tools/cplusplus-tools-utils/cplusplus-tools-utils.pri)

SOURCES += cplusplus-ast2png.cpp
