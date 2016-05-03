TEMPLATE = app
TARGET = leak2

CONFIG += debug

msvc: QMAKE_CXXFLAGS += -w44996
QT += core

macx:CONFIG -= app_bundle

SOURCES += main.cpp
