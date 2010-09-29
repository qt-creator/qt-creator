#-------------------------------------------------
#
# Project created by QtCreator 2010-02-24T10:21:38
#
#-------------------------------------------------

TARGET = qmldump
QT += declarative
CONFIG += console

### FIXME: only debug plugins are now supported.
CONFIG -= release
CONFIG += debug

TEMPLATE = app

SOURCES += main.cpp

OTHER_FILES += Info.plist
macx:QMAKE_INFO_PLIST = Info.plist
