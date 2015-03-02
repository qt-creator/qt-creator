TEMPLATE = app
TARGET = syscall

CONFIG += debug
win32-msvc*:QMAKE_CXXFLAGS += -w44700
else:QMAKE_CXXFLAGS = -O0 -Wno-uninitialized

QT += core

macx:CONFIG -= app_bundle

SOURCES += main.cpp
