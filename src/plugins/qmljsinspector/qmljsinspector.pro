TEMPLATE = lib
TARGET = QmlJSInspector
INCLUDEPATH += .
DEPENDPATH += .
QT += declarative network

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
qmljstoolbarcolorbox.h \
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
qmljstoolbarcolorbox.cpp \
qmljsinspectorclient.cpp \
qmljscontextcrumblepath.cpp \
qmljsinspectorsettings.cpp \
qmljspropertyinspector.cpp

include(../../libs/qmljsdebugclient/qmljsdebugclient-lib.pri)
include(../../../share/qtcreator/qml/qmljsdebugger/protocol/protocol.pri)

RESOURCES += qmljsinspector.qrc

include(../../qtcreatorplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/qmlprojectmanager/qmlprojectmanager.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/debugger/debugger.pri)
include(../../libs/utils/utils.pri)
