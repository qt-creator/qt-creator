TEMPLATE = lib
TARGET = QmlInspector

INCLUDEPATH += .
DEPENDPATH += .

include(components/qmldebugger.pri)

HEADERS += qmlinspectorplugin.h \
           qmlinspector.h \
           qmlinspectormode.h \
           inspectoroutputpane.h

SOURCES += qmlinspectorplugin.cpp \
           qmlinspectormode.cpp \
           inspectoroutputpane.cpp

OTHER_FILES += QmlInspector.pluginspec
RESOURCES += qmlinspector.qrc

include(../../qtcreatorplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)

