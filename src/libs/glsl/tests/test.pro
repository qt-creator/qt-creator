TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console
TARGET = glsl
DEPENDPATH += .
INCLUDEPATH += . ..

QT = core

# Input
SOURCES += main.cpp

include(../glsl-lib.pri)

