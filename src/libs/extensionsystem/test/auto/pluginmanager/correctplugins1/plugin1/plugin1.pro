TEMPLATE = lib
TARGET = plugin1

SOURCES += plugin1.cpp
HEADERS += plugin1.h

RELATIVEPATH = ../../../..
include(../../../../extensionsystem_test.pri)

LIBS += -L$${OUT_PWD}/../plugin2 -L$${OUT_PWD}/../plugin3 -lplugin2 -lplugin3

macx {
} else:unix {
    QMAKE_RPATHDIR += $${OUT_PWD}/../plugin2
    QMAKE_RPATHDIR += $${OUT_PWD}/../plugin3
}
