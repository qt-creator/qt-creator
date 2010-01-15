include(../../../../../src/plugins/qmldesigner/config.pri)
include(../../../../../src/plugins/qmldesigner/components/propertyeditor/propertyeditor.pri)

QT += testlib \
    script \
    declarative
DESTDIR = $$DESIGNER_BINARY_DIRECTORY
include(../../../../../src/plugins/qmldesigner/core/core.pri)
include(../../../../../src/shared/qmljs/qmljs.pri)
HEADERS+=../../../../../src/libs/utils/changeset.h
SOURCES+=../../../../../src/libs/utils/changeset.cpp
INCLUDEPATH+=../../../../../src/libs
DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB

DEPENDPATH += ../../../../../src/plugins/qmldesigner/core/include
DEPENDPATH += ..

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
