TEMPLATE = lib

SOURCES += plugin2.cpp
HEADERS += plugin2.h

OTHER_FILES = $$PWD/plugin.spec

include(../../../../../../qtcreator.pri)
include(../../../../../../src/libs/extensionsystem/extensionsystem.pri)
include(../../../../qttestrpath.pri)

COPYDIR = $$OUT_PWD
COPYFILES = $$OTHER_FILES
include(../../../copy.pri)

TARGET = $$qtLibraryName(plugin2)

DESTDIR = $$OUT_PWD/../lib
LIBS += -L$$OUT_PWD/../lib

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,$${OUT_PWD}/
}

