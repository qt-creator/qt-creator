TEMPLATE = lib

SOURCES += plugin3.cpp
HEADERS += plugin3.h
DEFINES += PLUGIN3_LIBRARY

OTHER_FILES = $$PWD/plugin.spec

QTC_LIB_DEPENDS += extensionsystem
include(../../../../qttest.pri)

COPYDIR = $$OUT_PWD
COPYFILES = $$OTHER_FILES
include(../../../copy.pri)

TARGET = $$qtLibraryName(plugin3)
CONFIG -= debug_and_release_target

LIBS += -L$$OUT_PWD/../plugin2
LIBS += -l$$qtLibraryName(plugin2)

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,$${OUT_PWD}/
} else:unix {
    QMAKE_RPATHDIR += $OUT_PWD/../lib
}
