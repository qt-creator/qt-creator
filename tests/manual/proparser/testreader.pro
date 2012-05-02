VPATH += ../../../src/shared/proparser
INCLUDEPATH += ../../../src/shared/proparser
DEPENDPATH += ../../../src/shared/proparser

TEMPLATE        = app
TARGET          = testreader

QT              -= gui

CONFIG          += qt warn_on console
CONFIG          -= app_bundle

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

SOURCES = main.cpp qmakeglobals.cpp profileparser.cpp profileevaluator.cpp proitems.cpp ioutils.cpp
HEADERS = qmakeglobals.h profileparser.h profileevaluator.h proitems.h ioutils.h

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
DEFINES += QT_USE_FAST_OPERATOR_PLUS QT_USE_FAST_CONCATENATION
DEFINES += PROEVALUATOR_CUMULATIVE PROEVALUATOR_INIT_PROPS
