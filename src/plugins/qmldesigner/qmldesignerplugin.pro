TEMPLATE = lib
TARGET = QmlDesigner

CONFIG += exceptions

INCLUDEPATH += $$PWD

include(../../qtcreatorplugin.pri)
include(../../private_headers.pri)
include(qmldesigner_dependencies.pri)

include(designercore/designercore-lib.pri)
include(components/componentcore/componentcore.pri)
include(components/integration/integration.pri)
include(components/propertyeditor/propertyeditor.pri)
include(components/formeditor/formeditor.pri)
include(components/itemlibrary/itemlibrary.pri)
include(components/navigator/navigator.pri)
include(components/pluginmanager/pluginmanager.pri)
include(components/stateseditor/stateseditor.pri)
include(components/resources/resources.pri)
include(qmldesignerplugin.pri)

DEFINES -= QT_NO_CAST_FROM_ASCII
