TEMPLATE = app
TARGET = uninit1

CONFIG += debug console
CONFIG -= qt
msvc: QMAKE_CXXFLAGS += -w44700
else: QMAKE_CXXFLAGS = -O0 -Wno-uninitialized

macx:CONFIG -= app_bundle

SOURCES += main.cpp
