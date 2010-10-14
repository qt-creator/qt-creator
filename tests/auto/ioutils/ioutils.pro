CONFIG += qtestlib testcase
TEMPLATE = app
CONFIG -= app_bundle

UTILS_PATH = ../../../src/shared/proparser

INCLUDEPATH += $$UTILS_PATH
DEPENDPATH += $$UTILS_PATH

SOURCES += \
    tst_ioutils.cpp

TARGET = tst_$$TARGET
