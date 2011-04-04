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
    callgrind/callgrindparser.h \
    callgrind/callgrindparsedata.h \
    callgrind/callgrindfunction.h \
    callgrind/callgrindfunction_p.h \
    callgrind/callgrindfunctioncycle.h \
    callgrind/callgrindfunctioncall.h \
    callgrind/callgrindcostitem.h \
    callgrind/callgrinddatamodel.h \
    callgrind/callgrindabstractmodel.h \
    callgrind/callgrindcallmodel.h \
    callgrind/callgrindcontroller.h \
    callgrind/callgrindcycledetection.h \
    callgrind/callgrindproxymodel.h \
    callgrind/callgrindstackbrowser.h \
    callgrind/callgrindrunner.h \
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
    callgrind/callgrindparser.cpp \
    callgrind/callgrindparsedata.cpp \
    callgrind/callgrindfunction.cpp \
    callgrind/callgrindfunctioncycle.cpp \
    callgrind/callgrindfunctioncall.cpp \
    callgrind/callgrindcostitem.cpp \
    callgrind/callgrindabstractmodel.cpp \
    callgrind/callgrinddatamodel.cpp \
    callgrind/callgrindcallmodel.cpp \
    callgrind/callgrindcontroller.cpp \
    callgrind/callgrindcycledetection.cpp \
    callgrind/callgrindproxymodel.cpp \
    callgrind/callgrindrunner.cpp \
    callgrind/callgrindstackbrowser.cpp \
    memcheck/memcheckrunner.cpp \
    valgrindrunner.cpp \
    valgrindprocess.cpp
