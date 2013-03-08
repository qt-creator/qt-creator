TEMPLATE = lib
DEFINES += MYPLUGIN_LIBRARY
SOURCES += testplugin.cpp
HEADERS += testplugin.h testplugin_global.h

OTHER_FILES += testplugin.xml

include(../../../../../qtcreator.pri)
include(../../../../../src/libs/extensionsystem/extensionsystem.pri)
include(../../../qttestrpath.pri)

COPYDIR = $$OUT_PWD
COPYFILES = $$PWD/testplugin.xml
include(../../copy.pri)

TARGET = $$qtLibraryName(test)
CONFIG -= debug_and_release_target
