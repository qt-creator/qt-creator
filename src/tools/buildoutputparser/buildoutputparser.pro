QTC_LIB_DEPENDS = utils
QTC_PLUGIN_DEPENDS = projectexplorer qtsupport qmakeprojectmanager
QT = core gui

include(../../qtcreatortool.pri)

TARGET = buildoutputparser

SOURCES = \
    main.cpp \
    outputprocessor.cpp

HEADERS = \
    outputprocessor.h
