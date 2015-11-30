QTC_LIB_DEPENDS = utils
QTC_PLUGIN_DEPENDS = projectexplorer qtsupport qmakeprojectmanager
QT = core gui

include(../../qtcreatortool.pri)

TARGET = buildoutputparser

win32|equals(TEST, 1):DEFINES += HAS_MSVC_PARSER

SOURCES = \
    main.cpp \
    outputprocessor.cpp

HEADERS = \
    outputprocessor.h
