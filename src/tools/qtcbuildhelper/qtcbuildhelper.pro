!win32: error("qtcbuildhelper is Windows only")

include(../../../qtcreator.pri)

TEMPLATE  = app
TARGET    = qtcbuildhelper
DESTDIR   = $$IDE_BIN_PATH

QT        -= qt core gui
CONFIG    += console warn_on

SOURCES   += main.cpp
LIBS      += -luser32 -lshell32

