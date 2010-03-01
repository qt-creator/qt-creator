#-------------------------------------------------
#
# Project created by QtCreator 2010-03-01T11:08:57
#
#-------------------------------------------------

# Utils lib requires gui, too.
QT       += core
QT       += gui

# -- Add creator 'utils' lib
CREATOR_LIB_LIB = ../../../lib/qtcreator
LIBS *= -L$$CREATOR_LIB_LIB
LIBS *= -l$$qtLibraryTarget(Utils)
QMAKE_RPATHDIR*=$$CREATOR_LIB_LIB
CREATOR_LIB_SRC = ../../../src/libs
INCLUDEPATH *= $$CREATOR_LIB_SRC

TARGET = process
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h
