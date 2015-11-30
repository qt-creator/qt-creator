TEMPLATE = app
TARGET = valgrind-fake

QT += network
QT -= gui widgets

macx:CONFIG -= app_bundle

HEADERS += outputgenerator.h
SOURCES += main.cpp \
    outputgenerator.cpp

include(../../../qtcreator.pri)
