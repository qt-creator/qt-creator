CONFIG += console
QT -= gui
TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = .

# Input
HEADERS += binpatch.h
SOURCES += binpatch.cpp qtlibspatchermain.cpp
