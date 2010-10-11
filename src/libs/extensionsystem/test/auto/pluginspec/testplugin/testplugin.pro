TEMPLATE = lib
TARGET = test
DEFINES += MYPLUGIN_LIBRARY
SOURCES += testplugin.cpp
HEADERS += testplugin.h testplugin_global.h

RELATIVEPATH = ../../..
include(../../../extensionsystem_test.pri)

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,$${OUT_PWD}/
}

