DEFINES += SYMBIANUTILS_INCLUDE_PRI
include(../../../src/shared/symbianutils/symbianutils.pri)

QT       += core gui network
TARGET = codaclient
TEMPLATE = app
CONFIG += console

SOURCES += main.cpp \
    codaclientapplication.cpp

HEADERS  += codaclientapplication.h
