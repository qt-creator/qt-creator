CREATORDIR=$$PWD/../../../../..

include($$CREATORDIR/src/plugins/qmldesigner/config.pri)
QT += testlib \
    script \
    declarative

include($$CREATORDIR/src/plugins/qmldesigner/core/core.pri)
include($$CREATORDIR/src/libs/qmljs/qmljs-lib.pri)
HEADERS+=$$CREATORDIR/src/libs/utils/changeset.h
SOURCES+=$$CREATORDIR/src/libs/utils/changeset.cpp

INCLUDEPATH+=$$CREATORDIR/src/libs

DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB QTCREATOR_TEST
DEFINES+=QTCREATORDIR=\\\"$$CREATORDIR\\\"

DEPENDPATH += ..
DEPENDPATH += $$CREATORDIR/src/plugins/qmldesigner/core/include

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
