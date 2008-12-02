TEMPLATE = lib
TARGET = plugin3

SOURCES += plugin3.cpp
HEADERS += plugin3.h

include(../../../../extensionsystem_test.pri)

LIBS += -L$${PWD}/../plugin2 -lplugin2

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,$${PWD}/
} else:unix {
    QMAKE_RPATHDIR += $${PWD}/../plugin2
}
