TEMPLATE = lib

SOURCES += plugin1.cpp
HEADERS += plugin1.h
DEFINES += PLUGIN1_LIBRARY

OTHER_FILES = $$PWD/plugin.xml

QTC_LIB_DEPENDS += extensionsystem
include(../../../../../../qtcreator.pri)
include(../../../../qttestrpath.pri)

COPYDIR = $$OUT_PWD
COPYFILES = $$OTHER_FILES
include(../../../copy.pri)

TARGET = $$qtLibraryName(plugin1)
CONFIG -= debug_and_release_target
