TEMPLATE = lib

SOURCES += plugin1.cpp
HEADERS += plugin1.h
DEFINES += PLUGIN1_LIBRARY

OTHER_FILES = $$PWD/plugin.json

QTC_LIB_DEPENDS += extensionsystem
include(../../../../../../qtcreator.pri)
include(../../../../qttestrpath.pri)

TARGET = $$qtLibraryName(plugin1)
CONFIG -= debug_and_release_target
