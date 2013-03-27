# This is a compile check for the dumpers only. Don't install the library!

include(../../../qtcreator.pri)

TEMPLATE = lib
TARGET = DebuggingHelper
DESTDIR = $$IDE_LIBRARY_PATH # /tmp would be better in some respect ...

linux-* {
CONFIG -= release
CONFIG += debug
# The following line works around a linker issue with gcc 4.1.2
QMAKE_CXXFLAGS *= -O2
}

true {
    QT = core
} else {
    DEFINES += USE_QT_GUI=1
    QT = core gui
}

SOURCES += ../../../share/qtcreator/dumper/dumper.cpp

