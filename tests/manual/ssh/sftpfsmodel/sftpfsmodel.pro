include(../ssh.pri)
include(../../../../src/shared/modeltest/modeltest.pri)

QT += widgets

TARGET=sftpfsmodel
SOURCES+=main.cpp window.cpp
HEADERS+=window.h
FORMS=window.ui
