TARGET = qmlpreviewplugin
TEMPLATE = lib
CONFIG += plugin

include(../../../../qtcreator.pri)\

include (../designercore/iwidgetplugin.pri)
include (../qmldesigner_dependencies.pri)

LIBS += -L$$IDE_PLUGIN_PATH
LIBS += -l$$qtLibraryName(QmlDesigner)
LIBS += -l$$qtLibraryName(ExtensionSystem)
LIBS += -l$$qtLibraryName(Core)
LIBS += -l$$qtLibraryName(ProjectExplorer)
LIBS += -l$$qtLibraryName(Utils)

include(qmlpreviewplugin.pri)
include(../plugindestdir.pri)
