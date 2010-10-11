TEMPLATE = lib
TARGET = plugin3

SOURCES += plugin3.cpp
HEADERS += plugin3.h

RELATIVEPATH = ../../../..
include(../../../../extensionsystem_test.pri)

LIBS += -L$${OUT_PWD}/../plugin2 -lplugin2

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,$${OUT_PWD}/
} else:unix {
    QMAKE_RPATHDIR += $${OUT_PWD}/../plugin2
}
