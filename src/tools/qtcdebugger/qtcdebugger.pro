include(../../shared/registryaccess/registryaccess.pri)
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = qtcdebugger
TEMPLATE = app
SOURCES += main.cpp

DESTDIR=../../../bin

target.path=$$QTC_PREFIX/bin
INSTALLS+=target

include(../../../qtcreator.pri)
app_info.input = $$PWD/../../app/app_version.h.in
app_info.output = $$OUT_PWD/app_version.h
QMAKE_SUBSTITUTES += app_info
