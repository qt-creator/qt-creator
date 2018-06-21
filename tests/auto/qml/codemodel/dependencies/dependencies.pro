QTC_LIB_DEPENDS += qmljs
QTC_PLUGIN_DEPENDS += qmljstools

include(../../../qttest.pri)

DEFINES+=QTCREATORDIR=\\\"$$IDE_SOURCE_TREE\\\"
DEFINES+=TESTSRCDIR=\\\"$$PWD\\\"

QT += core
QT -= gui

TARGET = tst_dependencies
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += tst_dependencies.cpp

DISTFILES += \
    samples/test_ApplicationWindow.qml \
    samples/test_Button.qml \
    samples/test_ColumnLayout.qml \
    samples/test_Component.qml \
    samples/test_GridLayout.qml \
    samples/test_ListModel.qml \
    samples/test_ListView.qml \
    samples/test_ListViewDelegate.qml \
    samples/test_LocalStorage.qml \
    samples/test_Material.qml \
    samples/test_MessageDialog.qml \
    samples/test_QtObject.qml \
    samples/test_RowLayout.qml \
    samples/test_SampleLib.qml \
    samples/test_StackLayout.qml \
    samples/test_Tests.qml \
    samples/test_Timer.qml \
    samples/test_Universal.qml \
    samples/test_Window.qml \
    samples/test_XmlListModel.qml \
    samples/test_CircularGauge.qml \
    samples/test_ColorDialog.qml \
    samples/test_Dialog.qml \
    samples/test_FileDialog.qml \
    samples/test_FontDialog.qml \
    samples/test_Gauge.qml \
    samples/test_PieMenu.qml \
    samples/test_StatusIndicator.qml \
    samples/test_Tumbler.qml \
    samples/test_TumblerColumn.qml \
    samples/test_VisualItemModel.qml

