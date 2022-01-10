QTC_LIB_DEPENDS += \
    utils \
    qmljs \
    qmleditorwidgets

QTC_LIB_DEPENDS += \
    utils \
    qmljs \
    qmleditorwidgets \
    cplusplus

QTC_PLUGIN_DEPENDS += \
    coreplugin \
    qmljseditor \
    qmakeprojectmanager \
    qmlprojectmanager

CONFIG -= qtquickcompiler

include(../../../qttest.pri)

QT += qml quick network core-private

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += printsupport
    !isEmpty(QT.webkitwidgets.name): QT += webkitwidgets webkit
    else: DEFINES += QT_NO_WEBKIT
} else {
    contains(QT_CONFIG, webkit): QT += webkit
}

unix:!openbsd:!osx: LIBS += -lrt # posix shared memory

DEFINES+=IDE_PLUGIN_PATH=$$system_quote(\"$$IDE_PLUGIN_PATH\")
DEFINES+=IDE_DATA_PATH=$$system_quote(\"$$IDE_DATA_PATH\")
DEFINES+=TESTSRCDIR=$$system_quote(\"$$_PRO_FILE_PWD_\")

DEFINES += QTCREATOR_TEST
DEFINES += QMLDESIGNER_TEST
DEFINES += QT_RESTRICTED_CAST_FROM_ASCII

INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/include
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore
INCLUDEPATH += $$IDE_SOURCE_TREE//share/qtcreator/qml/qmlpuppet
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/integration
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/componentcore
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/itemlibrary
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/navigator
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/stateseditor
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/formeditor
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/propertyeditor
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/debugview
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/edit3d
INCLUDEPATH *= $$IDE_SOURCE_TREE/src/libs/3rdparty
INCLUDEPATH *= $$IDE_SOURCE_TREE/src/libs/sqlite

include($$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/designercore-lib.pri)

CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += \
    ../testview.cpp \
    testrewriterview.cpp \
    tst_testcore.cpp \
    ../testconnectionmanager.cpp

HEADERS += \
    ../testview.h \
    testrewriterview.h \
    tst_testcore.h \
    ../testconnectionmanager.h

RESOURCES += ../data/testfiles.qrc
