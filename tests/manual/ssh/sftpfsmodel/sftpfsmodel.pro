include(../ssh.pri)
include(../../../../src/shared/modeltest/modeltest.pri)

QT += gui

TARGET=sftpfsmodel
SOURCES+=main.cpp window.cpp
HEADERS+=window.h
FORMS=window.ui
