CONFIG += qtestlib testcase
TEMPLATE = app
CONFIG -= app_bundle
DEFINES += QTCREATOR_UTILS_LIB

UTILS_PATH = ../../../src/libs/utils

INCLUDEPATH += $$UTILS_PATH/..

SOURCES += \
    tst_environment.cpp \
    $$UTILS_PATH/environment.cpp
HEADERS += \
    $$UTILS_PATH/environment.h \
    $$UTILS_PATH/utils_global.h

TARGET = tst_$$TARGET
