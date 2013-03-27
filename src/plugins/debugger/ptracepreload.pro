
include(../../../qtcreator.pri)

TEMPLATE = lib
TARGET = ptracepreload
CONFIG -= qt
DESTDIR = $$IDE_LIBRARY_PATH

LIBS += -ldl -lc

SOURCES = ptracepreload.c

