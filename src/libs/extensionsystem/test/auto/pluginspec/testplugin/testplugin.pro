TEMPLATE = lib
TARGET = test
DESTDIR = $${PWD}
DEFINES += MYPLUGIN_LIBRARY
SOURCES += testplugin.cpp
HEADERS += testplugin.h testplugin_global.h

include(../../../extensionsystem_test.pri)

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,$${PWD}/
}

