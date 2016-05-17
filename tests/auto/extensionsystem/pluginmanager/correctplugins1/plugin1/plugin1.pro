TEMPLATE = lib

SOURCES += plugin1.cpp
HEADERS += plugin1.h
DEFINES += PLUGIN1_LIBRARY

DISTFILES = $$PWD/plugin.json

QTC_LIB_DEPENDS += extensionsystem
include(../../../../qttest.pri)

TARGET = $$qtLibraryTargetName(plugin1)
CONFIG -= debug_and_release_target

LIBS += -L$$OUT_PWD/../plugin2 -L$$OUT_PWD/../plugin3
LIBS += -l$$qtLibraryName(plugin2) -l$$qtLibraryName(plugin3)

macx {
} else:unix {
    QMAKE_RPATHDIR += $$OUT_PWD/../lib
}
