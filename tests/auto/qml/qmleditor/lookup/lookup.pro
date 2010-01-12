TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
QT += testlib
include(../../../../../src/libs/qml/qml-lib.pri)
EDITOR_DIR=../../../../../src/plugins/qmleditor

INCLUDEPATH += $$EDITOR_DIR

TARGET=tst_$$TARGET

SOURCES += tst_lookup.cpp \
    $$EDITOR_DIR/qmllookupcontext.cpp

HEADERS += $$EDITOR_DIR/qmllookupcontext.h
RESOURCES += testfiles.qrc
