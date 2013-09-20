TEMPLATE = app
TARGET = uninit2

CONFIG += debug console
CONFIG -= qt
QMAKE_CXXFLAGS = -O0

macx:CONFIG -= app_bundle

SOURCES += main.cpp
