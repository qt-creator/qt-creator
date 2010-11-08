TEMPLATE = lib

SOURCES += plugin2.cpp
HEADERS += plugin2.h

OTHER_FILES = $$PWD/plugin.xml

include(../../../../../../../../qtcreator.pri)
include(../../../../../extensionsystem.pri)
include(../../../../../../../../tests/auto/qttestrpath.pri)

COPYDIR = $$OUT_PWD
COPYFILES = $$OTHER_FILES
include(../../../copy.pri)

TARGET = $$qtLibraryName(plugin2)
