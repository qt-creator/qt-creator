TEMPLATE = lib
CONFIG+=dll
TARGET = QmlJSDebugger

unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qtcreatorlibrary.pri)
include(qmljsdebugger-lib.pri)
