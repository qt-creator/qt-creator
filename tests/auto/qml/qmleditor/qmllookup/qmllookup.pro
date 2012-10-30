include(../../../../../qtcreator.pri)
TEMPLATE = app
QT += testlib
CONFIG += qt warn_on console depend_includepath testcase
include(../../../../../src/libs/qmljs/qmljs-lib.pri)
DEFINES += QML_BUILD_STATIC_LIB
EDITOR_DIR=../../../../../src/plugins/qmljseditor

INCLUDEPATH += $$EDITOR_DIR

SOURCES += tst_qmllookup.cpp

RESOURCES += testfiles.qrc

OTHER_FILES += \
    data/localIdLookup.qml \
    data/localScriptMethodLookup.qml \
    data/localScopeLookup.qml \
    data/localRootLookup.qml
