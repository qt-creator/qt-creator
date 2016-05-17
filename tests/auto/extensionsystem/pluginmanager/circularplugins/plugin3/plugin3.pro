TEMPLATE = lib

SOURCES += plugin3.cpp
HEADERS += plugin3.h
DEFINES += PLUGIN3_LIBRARY

ODISTFILES = $$PWD/plugin.json

QTC_LIB_DEPENDS += extensionsystem
include(../../../../../../qtcreator.pri)
include(../../../../qttestrpath.pri)

TARGET = $$qtLibraryTargetName(plugin3)
CONFIG -= debug_and_release_target
