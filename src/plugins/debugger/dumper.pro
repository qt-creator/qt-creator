include(../../qworkbenchlibrary.pri)

TEMPLATE = lib
TARGET = DebuggingHelper
CONFIG += shared
DESTDIR  = ../../../bin

linux-* {
CONFIG -= release
CONFIG += debug
# The following line works around a linker issue with gcc 4.1.2
QMAKE_CXXFLAGS *= -O2
}

SOURCES += ../../../share/qtcreator/gdbmacros/gdbmacros.cpp

