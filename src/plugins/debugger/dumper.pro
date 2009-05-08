# This is a compile check for the dumpers only. Don't install the library!

include(../../qtcreatorlibrary.pri)

TEMPLATE = lib
TARGET = DebuggingHelper
CONFIG += shared
DESTDIR = $$IDE_LIBRARY_PATH # /tmp would be better in some respect ...

linux-* {
CONFIG -= release
CONFIG += debug
# The following line works around a linker issue with gcc 4.1.2
QMAKE_CXXFLAGS *= -O2
}

SOURCES += ../../../share/qtcreator/gdbmacros/gdbmacros.cpp

