TEMPLATE = app

# this build works only in-source or inside a qtcreator shadow
# build if the test's shadow build directory is QTC-BUILD/tests/auto/qml/codemodel/basic

QTCREATOR_SOURCE=$$PWD/../../../../..
QTCREATOR_BUILD=$$OUT_PWD/../../../../..
# can we check that this is a valid build dir?

OUT_PWD_SAVE=$$OUT_PWD
OUT_PWD=QTCREATOR_BUILD
include($$QTCREATOR_SOURCE/qtcreator.pri)
OUT_PWD=$$OUT_PWD_SAVE


LIBS += -L$$IDE_PLUGIN_PATH/Nokia

unix: QMAKE_LFLAGS += \'-Wl,-rpath,$${IDE_LIBRARY_PATH}\' \'-Wl,-rpath,$${IDE_PLUGIN_PATH}/Nokia\'

QT += core network

CONFIG += qtestlib testcase

# DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB
DEFINES+=QTCREATORDIR=\\\"$$IDE_SOURCE_TREE\\\"
DEFINES+=QT_CREATOR QTCREATOR_TEST

include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/plugins/qmljstools/qmljstools.pri)


TARGET = tst_codemodel_basic
win32: DESTDIR = $$IDE_APP_PATH


CONFIG += console
CONFIG -= app_bundle
HEADERS += \
    metainfo.h

SOURCES += \
    tst_basic.cpp \
    metainfo.cpp
