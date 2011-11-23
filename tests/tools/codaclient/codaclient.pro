DEFINES += SYMBIANUTILS_INCLUDE_PRI

QT += network

include(../../../qtcreator.pri)
include(../../../src/shared/symbianutils/symbianutils.pri)

TARGET = codaclient
TEMPLATE = app
CONFIG += console

SOURCES += main.cpp \
    codaclientapplication.cpp

HEADERS  += codaclientapplication.h
