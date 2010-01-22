# -------------------------------------------------
# Project created by QtCreator 2010-01-22T10:11:10
# -------------------------------------------------
QT += core
TARGET = ccdb
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

# -- Add CDB core engine
CDB_CORE = ../../../src/plugins/debugger/cdb
include($$CDB_CORE/cdbcore.pri)
INCLUDEPATH *= $$CDB_CORE

# -- Add creator 'utils' lib
CREATOR_LIB_LIB = ../../../lib/qtcreator
LIBS *= -L$$CREATOR_LIB_LIB
LIBS *= -l$$qtLibraryTarget(Utilsd)
CREATOR_LIB_SRC = ../../../src/libs
INCLUDEPATH *= $$CREATOR_LIB_SRC

# -- App sources
SOURCES += main.cpp \
    cdbapplication.cpp \
    debugeventcallback.cpp \
    cdbpromptthread.cpp
HEADERS += cdbapplication.h \
    debugeventcallback.h \
    cdbpromptthread.h
