TEMPLATE = lib

SOURCES += plugin3.cpp
HEADERS += plugin3.h
DEFINES += PLUGIN3_LIBRARY

OTHER_FILES = $$PWD/plugin.xml

include(../../../../../../qtcreator.pri)
include(../../../../../../src/libs/extensionsystem/extensionsystem.pri)
include(../../../../qttestrpath.pri)

COPYDIR = $$OUT_PWD
COPYFILES = $$OTHER_FILES
include(../../../copy.pri)

TARGET = $$qtLibraryName(plugin3)
CONFIG -= debug_and_release_target
