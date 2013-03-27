include(../../../../../qtcreator.pri)
TEMPLATE = app
QT += script declarative webkit testlib
CONFIG += testcase
CONFIG += console
CONFIG -= app_bundle

include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/config.pri)
include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/designercore.pri)
include($$IDE_SOURCE_TREE/src/libs/qmljs/qmljs-lib.pri)
HEADERS+=$$IDE_SOURCE_TREE/src/libs/utils/changeset.h
SOURCES+=$$IDE_SOURCE_TREE/src/libs/utils/changeset.cpp
#DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB

INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/include
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/propertyeditor

SOURCES += testpropertyeditor.cpp
SOURCES += ../testview.cpp
HEADERS += testpropertyeditor.h
HEADERS += ../testview.h
RESOURCES += ../data/testfiles.qrc
