QT -= gui widgets
include(../qttest.pri)
include(../../../src/private_headers.pri)

greaterThan(QT_MAJOR_VERSION, 4) {
greaterThan(QT_MINOR_VERSION, 1): QT += core_private
else: QT += core-private
}

SOURCES += tst_offsets.cpp
