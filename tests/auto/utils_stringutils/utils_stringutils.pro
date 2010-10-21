include(../qttest.pri)
DEFINES += QTCREATOR_UTILS_LIB

UTILS_PATH = ../../../src/libs/utils

INCLUDEPATH += $$UTILS_PATH
# Input
SOURCES += tst_stringutils.cpp \
    $$UTILS_PATH/stringutils.cpp
HEADERS += $$UTILS_PATH/stringutils.h \
    $$UTILS_PATH/utils_global.h
