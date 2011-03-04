TEMPLATE = app
TARGET = valgrind-fake

QT += network xml

macx:CONFIG -= app_bundle

HEADERS += outputgenerator.h
SOURCES += main.cpp \
    outputgenerator.cpp
