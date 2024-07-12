add_qtc_plugin(ResourceEditor
  DEPENDS Qt::Xml
  PLUGIN_DEPENDS Core ProjectExplorer
  SOURCES
    qrceditor/qrceditor.cpp
    qrceditor/qrceditor.h
    qrceditor/resourcefile.cpp
    qrceditor/resourcefile_p.h
    resource_global.h
    resourceeditor.cpp
    resourceeditor.h
    resourceeditorconstants.h
    resourceeditorplugin.cpp
    resourceeditortr.h
    resourcenode.cpp
    resourcenode.h
)
