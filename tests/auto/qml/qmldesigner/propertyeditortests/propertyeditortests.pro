include(../../../../../qtcreator.pri)
include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/config.pri)
# include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/propertyeditor/propertyeditor.pri)

QT += testlib \
    script \
    declarative
# DESTDIR = $$DESIGNER_BINARY_DIRECTORY
include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/designercore.pri)
include($$IDE_SOURCE_TREE/src/libs/qmljs/qmljs-lib.pri)
HEADERS+=$$IDE_SOURCE_TREE/src/libs/utils/changeset.h
SOURCES+=$$IDE_SOURCE_TREE/src/libs/utils/changeset.cpp
INCLUDEPATH+=$$IDE_SOURCE_TREE/src/libs
#DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB

DEPENDPATH += ..
DEPENDPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/include
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore

TARGET = tst_propertyeditor

CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += testpropertyeditor.cpp
SOURCES += ../testview.cpp
HEADERS += testpropertyeditor.h
HEADERS += ../testview.h
RESOURCES += ../data/testfiles.qrc

HEADERS -= allpropertiesbox.h
SOURCES -= allpropertiesbox.cpp
