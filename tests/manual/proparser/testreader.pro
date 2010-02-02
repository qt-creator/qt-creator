VPATH += ../../../src/shared/proparser
INCLUDEPATH += ../../../src/shared/proparser

TEMPLATE        = app
TARGET          = testreader

QT              -= gui

CONFIG          += qt warn_on console
CONFIG          -= app_bundle

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

SOURCES = main.cpp profileevaluator.cpp proitems.cpp ioutils.cpp
HEADERS = profileevaluator.h proitems.h ioutils.h

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
