#-------------------------------------------------
#
# Project created by QtCreator 2010-03-01T11:08:57
#
#-------------------------------------------------

# Utils lib requires gui, too.
QT       += core
QT       += gui

include(../../../qtcreator.pri)

# -- Add creator 'utils' lib

LIBS *= -l$$qtLibraryName(Utils)

TARGET = process
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h
