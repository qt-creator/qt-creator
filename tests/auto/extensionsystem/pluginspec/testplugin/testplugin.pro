TEMPLATE = lib
DEFINES += MYPLUGIN_LIBRARY
SOURCES += testplugin.cpp
HEADERS += testplugin.h testplugin_global.h

OTHER_FILES += testplugin.xml

QTC_LIB_DEPENDS += extensionsystem
include(../../../../../qtcreator.pri)
include(../../../qttestrpath.pri)

COPYDIR = $$OUT_PWD
COPYFILES = $$PWD/testplugin.xml
include(../../copy.pri)

TARGET = $$qtLibraryName(test)
CONFIG -= debug_and_release_target
