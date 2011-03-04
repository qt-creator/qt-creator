TEMPLATE = lib
TARGET = Valgrind

include(../../qtcreatorlibrary.pri)
include(../utils/utils.pri)

TEMPLATE = lib

DEFINES += VALGRIND_LIBRARY

QT += network

HEADERS += valgrind_global.h \
    xmlprotocol/frame.h \
    xmlprotocol/parser.h \
    xmlprotocol/error.h \
    xmlprotocol/status.h \
    xmlprotocol/suppression.h \
    xmlprotocol/threadedparser.h \
    xmlprotocol/announcethread.h \
    xmlprotocol/stack.h \
    xmlprotocol/errorlistmodel.h \
    xmlprotocol/stackmodel.h \
    xmlprotocol/modelhelpers.h \
    memcheck/memcheckrunner.h \
    valgrindrunner.h \
    valgrindprocess.h

SOURCES += xmlprotocol/error.cpp \
    xmlprotocol/frame.cpp \
    xmlprotocol/parser.cpp \
    xmlprotocol/status.cpp \
    xmlprotocol/suppression.cpp \
    xmlprotocol/threadedparser.cpp \
    xmlprotocol/announcethread.cpp \
    xmlprotocol/stack.cpp \
    xmlprotocol/errorlistmodel.cpp \
    xmlprotocol/stackmodel.cpp \
    xmlprotocol/modelhelpers.cpp \
    memcheck/memcheckrunner.cpp \
    valgrindrunner.cpp \
    valgrindprocess.cpp
