TEMPLATE = lib
CONFIG += dll
TARGET = QmlJS
DEFINES += QML_BUILD_LIB QT_CREATOR

unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qtcreatorlibrary.pri)
include(qmljs-lib.pri)
