include(../../../../../qtcreator.pri)
include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/config.pri)

QT += script \
    network \
    declarative

CONFIG += qtestlib testcase

# DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB
DEFINES+=QTCREATORDIR=\\\"$$IDE_SOURCE_TREE\\\"
DEFINES+=QT_CREATOR QTCREATOR_TEST

DEPENDPATH += ..
DEPENDPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/include
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore

include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/designercore.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils-lib.pri)
include($$IDE_SOURCE_TREE/src/libs/qmljs/qmljs-lib.pri)

TARGET = tst_qmldesigner_core

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
