CONFIG += qtestlib testcase
TEMPLATE = app
CONFIG -= app_bundle
DEFINES += QTCREATOR_UTILS_LIB

win32:DEFINES += _CRT_SECURE_NO_WARNINGS

UTILS_PATH = ../../../src/libs/utils

INCLUDEPATH += $$UTILS_PATH/..
DEPENDPATH += $$UTILS_PATH/..

SOURCES += \
    tst_qtcprocess.cpp \
    $$UTILS_PATH/qtcprocess.cpp \
    $$UTILS_PATH/stringutils.cpp \
    $$UTILS_PATH/environment.cpp
HEADERS += \
    $$UTILS_PATH/qtcprocess.h \
    $$UTILS_PATH/environment.h \
    $$UTILS_PATH/stringutils.h \
    $$UTILS_PATH/utils_global.h

TARGET = tst_$$TARGET
