TEMPLATE = lib
TARGET = QmlJSInspector
INCLUDEPATH += .
DEPENDPATH += .
QT += network
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += quick1
} else {
    QT += declarative
}

DEFINES += QMLJSINSPECTOR_LIBRARY

HEADERS += \
qmljsprivateapi.h \
qmljsinspector_global.h \
qmljsinspectorconstants.h \
qmljsinspectorplugin.h \
qmljsclientproxy.h \
qmljsinspector.h \
qmljsinspectortoolbar.h \
qmljslivetextpreview.h \
qmljsinspectorclient.h \
qmljscontextcrumblepath.h \
qmljsinspectorsettings.h \
qmljspropertyinspector.h

SOURCES += \
qmljsinspectorplugin.cpp \
qmljsclientproxy.cpp \
qmljsinspector.cpp \
qmljsinspectortoolbar.cpp \
qmljslivetextpreview.cpp \
qmljsinspectorclient.cpp \
qmljscontextcrumblepath.cpp \
qmljsinspectorsettings.cpp \
qmljspropertyinspector.cpp

include(../../../share/qtcreator/qml/qmljsdebugger/protocol/protocol.pri)

RESOURCES += qmljsinspector.qrc

include(../../qtcreatorplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/qmlprojectmanager/qmlprojectmanager.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/debugger/debugger.pri)
include(../../libs/utils/utils.pri)
include(../../libs/qmljsdebugclient/qmljsdebugclient.pri)
