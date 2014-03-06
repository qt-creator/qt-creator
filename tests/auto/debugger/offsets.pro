QTC_LIB_DEPENDS += utils
QT -= gui widgets
greaterThan(QT_MAJOR_VERSION, 4): QT += core_private
include(../qttest.pri)

SOURCES += tst_offsets.cpp
