TEMPLATE = app
TARGET = debuggertest
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = ..

# Input
SOURCES += ../app.cpp
QT += network

mesage("this says <foo & bar>")
