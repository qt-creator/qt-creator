TEMPLATE = app
QT = core \
    network
QT -= gui
include($$PWD/trklauncher.pri)
DEFINES += DEBUG_TRK=1
win32:CONFIG += console
SOURCES += main_launcher.cpp
