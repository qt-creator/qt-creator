include(../ssh.pri)
include(../../../../src/shared/modeltest/modeltest.pri)

QT += gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET=sftpfsmodel
SOURCES+=main.cpp window.cpp
HEADERS+=window.h
FORMS=window.ui
