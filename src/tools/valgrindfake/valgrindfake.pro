TEMPLATE = app
TARGET = valgrind-fake

QT += network xml
QT -= gui widgets

macx:CONFIG -= app_bundle

HEADERS += outputgenerator.h
SOURCES += main.cpp \
    outputgenerator.cpp

include(../../../qtcreator.pri)
