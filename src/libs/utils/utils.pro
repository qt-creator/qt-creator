TEMPLATE = lib
TARGET = Utils
QT += gui \
    network

CONFIG += dll
include($$PWD/../../qtcreatorlibrary.pri)

include(utils-lib.pri)

HEADERS += \
    proxyaction.h

SOURCES += \
    proxyaction.cpp
