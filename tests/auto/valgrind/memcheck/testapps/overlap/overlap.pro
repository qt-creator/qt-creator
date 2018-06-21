TEMPLATE = app
TARGET = overlap

CONFIG += debug console
CONFIG -= qt
!msvc: QMAKE_CXXFLAGS *= -O0 -fno-builtin

macx:CONFIG -= app_bundle

SOURCES += main.cpp
