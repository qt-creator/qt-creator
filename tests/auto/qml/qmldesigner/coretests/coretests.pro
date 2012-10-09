include(../../../qttest.pri)

QTCREATOR_SOURCE=$$PWD/../../../../..
QTCREATOR_BUILD=$$OUT_PWD/../../../../..
# can we check that this is a valid build dir?

OUT_PWD_SAVE=$$OUT_PWD
OUT_PWD=QTCREATOR_BUILD
include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/config.pri)
OUT_PWD=$$OUT_PWD_SAVE

LIBS += -L$$IDE_PLUGIN_PATH/QtProject

unix: QMAKE_LFLAGS += \'-Wl,-rpath,$${IDE_LIBRARY_PATH}\' \'-Wl,-rpath,$${IDE_PLUGIN_PATH}/QtProject\'

QT += script \
    network \
    declarative \
    webkit

# DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB
DEFINES+=QTCREATORDIR=\\\"$$IDE_BUILD_TREE\\\"
DEFINES+=QT_CREATOR QTCREATOR_TEST

INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/include
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore

include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/designercore.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/plugins/qmljstools/qmljstools.pri)

CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += \
    ../testview.cpp \
    testrewriterview.cpp \
    tst_testcore.cpp
HEADERS += \
    ../testview.h \
    testrewriterview.h \
    tst_testcore.h
RESOURCES += ../data/testfiles.qrc
