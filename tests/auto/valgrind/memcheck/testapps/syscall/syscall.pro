TEMPLATE = app
TARGET = syscall

CONFIG += debug
QMAKE_CXXFLAGS = -O0

QT += core

macx:CONFIG -= app_bundle

SOURCES += main.cpp
