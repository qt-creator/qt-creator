TEMPLATE = lib

SOURCES += plugin1.cpp
HEADERS += plugin1.h

OTHER_FILES = $$PWD/plugin.xml

include(../../../../../../../../qtcreator.pri)
include(../../../../../extensionsystem.pri)
include(../../../../../../../../tests/auto/qttestrpath.pri)

COPYDIR = $$OUT_PWD
COPYFILES = $$OTHER_FILES
include(../../../copy.pri)

TARGET = $$qtLibraryName(plugin1)
