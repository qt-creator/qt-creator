REL_PATH_TO_SRC = ../../../src

QT += testlib

SOURCES += \
        tst_icheckbuild.cpp

DEFINES += ICHECK_BUILD ICHECK_APP_BUILD

INCLUDEPATH += . \
    $$REL_PATH_TO_SRC/../ \
    $$REL_PATH_TO_SRC/global \
    $$REL_PATH_TO_SRC/plugins \
    $$REL_PATH_TO_SRC/libs \
    $$REL_PATH_TO_SRC/shared/cplusplus \
    $$REL_PATH_TO_SRC/libs/cplusplus

TARGET=tst_$$TARGET

include(./ichecklib.pri)
HEADERS += ./ichecklib.h \
           ./ichecklib_global.h \
           ./ParseManager.h
SOURCES += ./ichecklib.cpp \
           ./ParseManager.cpp
