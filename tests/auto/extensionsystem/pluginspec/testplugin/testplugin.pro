include(../../../../../../../qtcreator.pri)

TEMPLATE = lib
TARGET = $$qtLibraryName(test)
DEFINES += MYPLUGIN_LIBRARY
SOURCES += testplugin.cpp
HEADERS += testplugin.h testplugin_global.h

OTHER_FILES += testplugin.xml

include(../../../../extensionsystem.pri)

include(../../../../../../../tests/auto/qttestrpath.pri)

COPYDIR = $$OUT_PWD
COPYFILES = $$PWD/testplugin.xml
include(../../copy.pri)
