TEMPLATE = app
QT = core \
    network
QT -= gui
include($$PWD/trklauncher.pri)
DEFINES += DEBUG=0
win32:CONFIG += console
SOURCES += main_launcher.cpp
