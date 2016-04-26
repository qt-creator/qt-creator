TEMPLATE = app
TARGET = leak3

CONFIG += debug

QT += core

osx: CONFIG -= app_bundle
msvc: QMAKE_CXXFLAGS += -w44996

SOURCES += main.cpp
