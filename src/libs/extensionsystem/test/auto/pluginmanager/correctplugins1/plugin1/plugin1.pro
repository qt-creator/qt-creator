TEMPLATE = lib
TARGET = plugin1

SOURCES += plugin1.cpp
HEADERS += plugin1.h

include(../../../../extensionsystem_test.pri)

LIBS += -L$${PWD}/../plugin2 -L$${PWD}/../plugin3 -lplugin2 -lplugin3

macx {
} else:unix {
    QMAKE_RPATHDIR += $${PWD}/../plugin2
    QMAKE_RPATHDIR += $${PWD}/../plugin3
}
