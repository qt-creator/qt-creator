TEMPLATE = lib
TARGET = plugin2

SOURCES += plugin2.cpp
HEADERS += plugin2.h

RELATIVEPATH = ../../../..
include(../../../../extensionsystem_test.pri)

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,$${OUT_PWD}/
}

