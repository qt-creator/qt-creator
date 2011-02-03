
include(../../qtcreatorlibrary.pri)

TEMPLATE = lib
TARGET = ptracepreload
CONFIG += shared
CONFIG -= qt
DESTDIR = $$IDE_LIBRARY_PATH

QMAKE_LFLAGS *= -nostdlib -ldl -lc

SOURCES = ptracepreload.c

