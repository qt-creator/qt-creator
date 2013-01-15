IDE_SOURCE_TREE=$$PWD/../../../../..
IDE_BUILD_TREE=$$OUT_PWD/../../../../..
# can we check that this is a valid build dir?
OUT_PWD_SAVE=$$OUT_PWD
OUT_PWD=IDE_BUILD_TREE
include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/config.pri)
include(../../../qttest.pri)
OUT_PWD=$$OUT_PWD_SAVE
LIBS += -L$$IDE_PLUGIN_PATH/QtProject
LIBS += -L$$IDE_LIBRARY_PATH

unix: QMAKE_LFLAGS += \'-Wl,-rpath,$${IDE_LIBRARY_PATH}\' \'-Wl,-rpath,$${IDE_PLUGIN_PATH}/QtProject\'

QT += script \
    network

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += printsupport
    !isEmpty(QT.webkitwidgets.name): QT += webkitwidgets webkit
    else: DEFINES += QT_NO_WEBKIT
} else {
    contains(QT_CONFIG, webkit): QT += webkit
}


# DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB
DEFINES+=QTCREATORDIR=\\\"$$IDE_BUILD_TREE\\\"
DEFINES+=QT_CREATOR QTCREATOR_TEST QMLDESIGNER_TEST

INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/include
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore
INCLUDEPATH += $$IDE_SOURCE_TREE//share/qtcreator/qml/qmlpuppet
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner


include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/designercore-lib.pri)
include($$IDE_SOURCE_TREE/src/plugins/qmljstools/qmljstools.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/libs/qmljs/qmljs.pri)
include($$IDE_SOURCE_TREE/src/libs/cplusplus/cplusplus.pri)

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
