TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
QT += testlib
include(../../../../../src/libs/qmljs/qmljs-lib.pri)
DEFINES += QML_BUILD_STATIC_LIB
EDITOR_DIR=../../../../../src/plugins/qmljseditor

INCLUDEPATH += $$EDITOR_DIR

TARGET=tst_$$TARGET

SOURCES += tst_lookup.cpp \
    $$EDITOR_DIR/qmllookupcontext.cpp

HEADERS += $$EDITOR_DIR/qmllookupcontext.h
RESOURCES += testfiles.qrc

OTHER_FILES += \
    data/localIdLookup.qml \
    data/localScriptMethodLookup.qml \
    data/localScopeLookup.qml \
    data/localRootLookup.qml
