TEMPLATE = app
QT = core
include($$PWD/trklauncher.pri)
win32:CONFIG += console
SOURCES += main_launcher.cpp
