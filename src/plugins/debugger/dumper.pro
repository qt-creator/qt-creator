
TEMPLATE = lib
TARGET = DebuggingHelper
CONFIG += shared
DESTDIR  = ../../../bin
include(../../qworkbenchlibrary.pri)

linux-* {
CONFIG -= release
CONFIG += debug
}

SOURCES += ../../../share/qtcreator/gdbmacros/gdbmacros.cpp

