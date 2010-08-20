#-------------------------------------------------
#
# Project created by QtCreator 2010-07-01T09:44:44
#
#-------------------------------------------------

INCLUDEPATH += ../../../src/plugins
CREATORLIBPATH = ../../../lib/qtcreator
PLUGINPATH=$$CREATORLIBPATH/plugins/Nokia
LIBS *= -L$$PLUGINPATH -lCore
LIBS *= -L$$CREATORLIBPATH
include (../../../qtcreator.pri)
include (../../../src/plugins/coreplugin/coreplugin_dependencies.pri)

QT       += core

QT       -= gui

TARGET = ssh
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp
