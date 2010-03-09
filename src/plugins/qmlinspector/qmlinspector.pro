TEMPLATE = lib
TARGET = QmlInspector

INCLUDEPATH += .
DEPENDPATH += .

include(../private_headers.pri)
include(components/qmldebugger.pri)

DEFINES += QMLINSPECTOR_LIBRARY

HEADERS += qmlinspectorplugin.h \
           qmlinspectorconstants.h \
           qmlinspector.h \
           inspectoroutputwidget.h \
           qmlinspector_global.h

SOURCES += qmlinspectorplugin.cpp \
           qmlinspector.cpp \
           inspectoroutputwidget.cpp

OTHER_FILES += QmlInspector.pluginspec
RESOURCES += qmlinspector.qrc

include(../../qtcreatorplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/qmlprojectmanager/qmlprojectmanager.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/debugger/debugger.pri)
