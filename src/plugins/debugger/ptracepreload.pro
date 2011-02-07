
include(../../qtcreatorlibrary.pri)

TEMPLATE = lib
TARGET = ptracepreload
CONFIG += shared
CONFIG -= qt
DESTDIR = $$IDE_LIBRARY_PATH

QMAKE_LFLAGS -= -Wl,--as-needed
QMAKE_LFLAGS *= -nostdlib
LIBS += -ldl -lc

SOURCES = ptracepreload.c

