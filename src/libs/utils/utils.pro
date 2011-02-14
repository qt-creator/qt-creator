TEMPLATE = lib
TARGET = Utils
QT += gui \
    network

CONFIG += dll
include($$PWD/../../qtcreatorlibrary.pri)
include($$PWD/../3rdparty/botan/botan.pri)

include(utils-lib.pri)

HEADERS += \
    proxyaction.h

SOURCES += \
    proxyaction.cpp
