TEMPLATE = app
TARGET = valgrind-fake

QT += network xml

macx:CONFIG -= app_bundle

isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/../../shared/qtcreator_pch.h

HEADERS += outputgenerator.h
SOURCES += main.cpp \
    outputgenerator.cpp
