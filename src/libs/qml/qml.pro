TEMPLATE = lib
CONFIG += dll
TARGET = Qml
DEFINES += QML_BUILD_LIB QT_CREATOR

unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qtcreatorlibrary.pri)
include(qml-lib.pri)
