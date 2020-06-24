#-------------------------------------------------
#
# Project created by QtCreator 2017-01-03T08:16:50
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_derivedtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += tst_derivedtest.cpp \
    origin.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS += \
    origin.h
