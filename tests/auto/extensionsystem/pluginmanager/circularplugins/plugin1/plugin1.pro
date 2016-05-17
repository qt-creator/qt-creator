TEMPLATE = lib

SOURCES += plugin1.cpp
HEADERS += plugin1.h
DEFINES += PLUGIN1_LIBRARY

DISTFILES = $$PWD/plugin.json

QTC_LIB_DEPENDS += extensionsystem
include(../../../../../../qtcreator.pri)
include(../../../../qttestrpath.pri)

TARGET = $$qtLibraryTargetName(plugin1)
CONFIG -= debug_and_release_target
