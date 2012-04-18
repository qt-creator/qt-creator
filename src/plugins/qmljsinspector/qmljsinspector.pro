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
qmltoolsclient.h \
qmljscontextcrumblepath.h \
qmljsinspectorsettings.h \
qmljspropertyinspector.h

SOURCES += \
qmljsinspectorplugin.cpp \
qmljsclientproxy.cpp \
qmljsinspector.cpp \
qmljsinspectortoolbar.cpp \
qmljslivetextpreview.cpp \
qmltoolsclient.cpp \
qmljscontextcrumblepath.cpp \
qmljsinspectorsettings.cpp \
qmljspropertyinspector.cpp

include(../../../share/qtcreator/qml/qmljsdebugger/protocol/protocol.pri)

RESOURCES += qmljsinspector.qrc

include(../../qtcreatorplugin.pri)
include(../../libs/qmldebug/qmldebug.pri)
include(../../libs/qmleditorwidgets/qmleditorwidgets.pri)

include(../../plugins/debugger/debugger.pri)
include(../../plugins/qmljstools/qmljstools.pri)
