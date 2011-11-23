include(../../shared/registryaccess/registryaccess.pri)
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = qtcdebugger
TEMPLATE = app
SOURCES += main.cpp

DESTDIR=../../../bin
