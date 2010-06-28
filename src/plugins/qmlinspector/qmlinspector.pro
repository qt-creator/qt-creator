TEMPLATE = lib
TARGET = QmlInspector

INCLUDEPATH += .
DEPENDPATH += .

include(../../private_headers.pri)

DEFINES += QMLINSPECTOR_LIBRARY

HEADERS += qmlinspectorplugin.h \
           qmlinspectorconstants.h \
           qmlinspector_global.h \
           inspectorcontext.h \

SOURCES += qmlinspectorplugin.cpp \
           inspectorcontext.cpp

OTHER_FILES += QmlInspector.pluginspec
RESOURCES += qmlinspector.qrc

include(../../qtcreatorplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/qmlprojectmanager/qmlprojectmanager.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/debugger/debugger.pri)
