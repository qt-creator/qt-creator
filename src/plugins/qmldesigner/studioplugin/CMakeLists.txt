add_qtc_plugin(StudioPlugin
    PLUGIN_CLASS StudioPlugin
    CONDITION TARGET QmlDesigner
    DEPENDS Core QmlDesigner Utils ProjectExplorer
    SOURCES
        studioplugin.cpp
        studioplugin.h
        studioplugin.qrc
    PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
)
