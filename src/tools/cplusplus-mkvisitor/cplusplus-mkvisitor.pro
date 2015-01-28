QT = core gui
macx:CONFIG -= app_bundle
win32:CONFIG += console
TEMPLATE = app
TARGET = cplusplus-mkvisitor
DESTDIR = ./

include(../../../qtcreator.pri)
include(../../libs/cplusplus/cplusplus-lib.pri)
include(../../../src/tools/cplusplus-tools-utils/cplusplus-tools-utils.pri)

DEFINES += PATH_AST_H=\\\"$$PWD/../../libs/3rdparty/cplusplus/AST.h\\\"
SOURCES += cplusplus-mkvisitor.cpp
