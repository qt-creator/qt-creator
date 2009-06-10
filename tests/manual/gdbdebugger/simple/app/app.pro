TEMPLATE = app
TARGET = debuggertest
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = ..

# Input
SOURCES += ../app.cpp

# SOURCES += ../../../../../share/qtcreator/gdbmacros/gdbmacros.cpp
QT += network
message("this says <foo & bar>")
