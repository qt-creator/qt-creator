#-------------------------------------------------
#
# Project created by QtCreator 2010-02-24T10:21:38
#
#-------------------------------------------------

TARGET = qmldump
QT += declarative
CONFIG += console

TEMPLATE = app

SOURCES += main.cpp \
    qmlstreamwriter.cpp

# generation of Info.plist from Info.plist.in is handled by static.pro
# compiling this project directly from the Qt Creator source tree does not work
macx:QMAKE_INFO_PLIST = Info.plist

HEADERS += \
    qmlstreamwriter.h
