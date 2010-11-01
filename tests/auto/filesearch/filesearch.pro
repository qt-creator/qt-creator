CONFIG += qtestlib testcase
TEMPLATE = app
CONFIG -= app_bundle
DEFINES += QTCREATOR_UTILS_LIB

include(../../../qtcreator.pri)

UTILS_PATH = $$IDE_SOURCE_TREE/src/libs/utils

INCLUDEPATH += $$UTILS_PATH
# Input
SOURCES += tst_filesearch.cpp \
    $$UTILS_PATH/filesearch.cpp
HEADERS += $$UTILS_PATH/filesearch.h \
    $$UTILS_PATH/utils_global.h

TARGET=tst_$$TARGET

OTHER_FILES += testfile.txt

RESOURCES += \
    tst_filesearch.qrc
