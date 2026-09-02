QT += testlib
QT -= gui

CONFIG += qt warn_on depend_includepath staticlib

TEMPLATE = lib

TARGET = inlibtests

SOURCES += \
    tst_inlib.cpp

HEADERS += \
    tst_inlib.h
