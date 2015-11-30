TEMPLATE = app
TARGET = leak2

CONFIG += debug

win32-msvc*:QMAKE_CXXFLAGS += -w44996
QT += core

macx:CONFIG -= app_bundle

SOURCES += main.cpp
