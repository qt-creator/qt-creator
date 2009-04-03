TEMPLATE = app
TARGET = debuggertest
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = ..

# Input
SOURCES += ../app.cpp
QT += network

message("this says <foo & bar>")
