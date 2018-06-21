QT       -= gui

include(../qtbreakpad.pri)

unix:TARGET = testapp.bin
win32:TARGET = testapp

## install_name_tool -change libQtBreakpad.1.dylib @executable_path/../lib/qtbreakpad/libQtBreakpad.dylib

DESTDIR = ../bin

CONFIG   += console release c++11

SOURCES += main.cpp
