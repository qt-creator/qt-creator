TEMPLATE = lib
CONFIG += dll
TARGET = QmlJS
DEFINES += QMLJS_BUILD_DIR QT_CREATOR

unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qtcreatorlibrary.pri)
include(qmljs-lib.pri)
