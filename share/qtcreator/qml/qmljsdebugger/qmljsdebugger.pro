# This file is part of Qt Creator
# It enables debugging of Qt Quick applications 

TEMPLATE = lib
CONFIG+=dll
TARGET = QmlJSDebugger

unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../../../src/qtcreatorlibrary.pri)
include(qmljsdebugger-lib.pri)
