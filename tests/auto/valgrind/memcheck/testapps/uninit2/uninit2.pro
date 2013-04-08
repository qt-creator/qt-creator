TEMPLATE = app
TARGET = uninit2

CONFIG += debug
QMAKE_CXXFLAGS = -O0

QT -= core gui

macx:CONFIG -= app_bundle

SOURCES += main.cpp
