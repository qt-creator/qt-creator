# This file is part of Qt Creator
# It enables debugging of Qt Quick applications 

TEMPLATE = lib
CONFIG += staticlib create_prl
QT += declarative script

DEFINES += BUILD_QMLJSDEBUGGER_STATIC_LIB

unix:QMAKE_CXXFLAGS_DEBUG += -O3

DESTDIR = $$PWD
TARGET=QmlJSDebugger
CONFIG(debug, debug|release) {
   windows:TARGET=QmlJSDebuggerd
}

include(qmljsdebugger-src.pri)
