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
           qmlinspector_global.h \
    inspectorcontext.h \
    startexternalqmldialog.h \
    inspectorsettings.h

SOURCES += qmlinspectorplugin.cpp \
           qmlinspector.cpp \
           inspectoroutputwidget.cpp \
    inspectorcontext.cpp \
    startexternalqmldialog.cpp \
    inspectorsettings.cpp

OTHER_FILES += QmlInspector.pluginspec
RESOURCES += qmlinspector.qrc

FORMS += \
    startexternalqmldialog.ui

include(../../qtcreatorplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/qmlprojectmanager/qmlprojectmanager.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/debugger/debugger.pri)
