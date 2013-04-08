TEMPLATE = app
TARGET = overlap

CONFIG += debug
QMAKE_CXXFLAGS = -O0 -fno-builtin

QT -= core gui

macx:CONFIG -= app_bundle

SOURCES += main.cpp
