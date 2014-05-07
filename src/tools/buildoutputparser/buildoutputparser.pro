TEMPLATE = app
TARGET = buildoutputparser
QTC_LIB_DEPENDS = utils
QTC_PLUGIN_DEPENDS = projectexplorer qtsupport qmakeprojectmanager

QT = core gui
CONFIG += console
CONFIG -= app_bundle

include(../../../qtcreator.pri)
include(../../rpath.pri)

win32|equals(TEST, 1):DEFINES += HAS_MSVC_PARSER

DESTDIR = $$IDE_BIN_PATH
target.path = /bin
INSTALLS += target

SOURCES = \
    main.cpp \
    outputprocessor.cpp

HEADERS = \
    outputprocessor.h
