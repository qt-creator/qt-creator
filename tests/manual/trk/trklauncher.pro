TEMPLATE = app
QT = core network
QT -= gui

win32:CONFIG += console
win32:DEFINES += USE_NATIVE

SOURCES = launcher.cpp \
    trkutils.cpp
HEADERS = trkutils.h
