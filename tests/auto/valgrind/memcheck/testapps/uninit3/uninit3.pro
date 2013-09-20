TEMPLATE = app
TARGET = uninit3

CONFIG += debug console
CONFIG -= qt
QMAKE_CXXFLAGS = -O0

macx:CONFIG -= app_bundle

SOURCES += main.cpp
