include(../../../../../qtcreator.pri)

include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/config.pri)

QT += testlib \
    script \
    declarative

# DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB
DEFINES+=QTCREATORDIR=\\\"$$CREATORDIR\\\"
DEFINES+=QT_CREATOR QTCREATOR_TEST

DEPENDPATH += ..
DEPENDPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/include
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore

include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/designercore.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/libs/qmljs/qmljs.pri)

TARGET = tst_qmldesigner_core

CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += testcore.cpp \
    ../testview.cpp \
    testrewriterview.cpp
HEADERS += testcore.h \
    ../testview.h \
    testrewriterview.h
RESOURCES += ../data/testfiles.qrc
