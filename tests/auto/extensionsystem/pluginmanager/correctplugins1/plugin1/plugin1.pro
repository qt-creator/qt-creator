TEMPLATE = lib

SOURCES += plugin1.cpp
HEADERS += plugin1.h

OTHER_FILES = $$PWD/plugin.spec

include(../../../../qttest.pri)
include(../../../../../../src/libs/extensionsystem/extensionsystem.pri)

COPYDIR = $$OUT_PWD
COPYFILES = $$OTHER_FILES
include(../../../copy.pri)

TARGET = $$qtLibraryName(plugin1)
CONFIG -= debug_and_release_target

LIBS += -L$$OUT_PWD/../plugin2 -L$$OUT_PWD/../plugin3
LIBS += -l$$qtLibraryName(plugin2) -l$$qtLibraryName(plugin3)

macx {
} else:unix {
    QMAKE_RPATHDIR += $$OUT_PWD/../lib
}
