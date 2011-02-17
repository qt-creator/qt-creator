TEMPLATE = lib
TARGET = Utils
QT += gui \
    network

CONFIG += dll
include(../../qtcreatorlibrary.pri)
include(utils_dependencies.pri)

include(utils-lib.pri)

HEADERS += \
    proxyaction.h

SOURCES += \
    proxyaction.cpp
