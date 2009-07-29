TEMPLATE = app
QT = core network
QT -= gui

DEFINES += DEBUG=0
win32:CONFIG += console
win32:DEFINES += USE_NATIVE

SOURCES = launcher.cpp \
    trkutils.cpp
HEADERS = trkutils.h
