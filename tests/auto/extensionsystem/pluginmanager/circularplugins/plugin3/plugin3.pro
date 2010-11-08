TEMPLATE = lib

SOURCES += plugin3.cpp
HEADERS += plugin3.h

OTHER_FILES = $$PWD/plugin.xml

include(../../../../../../../../qtcreator.pri)
include(../../../../../extensionsystem.pri)
include(../../../../../../../../tests/auto/qttestrpath.pri)

COPYDIR = $$OUT_PWD
COPYFILES = $$OTHER_FILES
include(../../../copy.pri)

TARGET = $$qtLibraryName(plugin3)
