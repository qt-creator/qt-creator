TEMPLATE = lib
DEFINES += MYPLUGIN_LIBRARY
SOURCES += testplugin.cpp
HEADERS += testplugin.h testplugin_global.h

DISTFILES += testplugin.json

QTC_LIB_DEPENDS += extensionsystem
include(../../../../../qtcreator.pri)
include(../../../qttestrpath.pri)

TARGET = $$qtLibraryName(test)
CONFIG -= debug_and_release_target
