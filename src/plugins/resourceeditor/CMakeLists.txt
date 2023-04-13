add_qtc_plugin(ResourceEditor
  DEPENDS Qt::Xml
  PLUGIN_DEPENDS Core ProjectExplorer
  SOURCES
    qrceditor/qrceditor.cpp qrceditor/qrceditor.h
    qrceditor/resourcefile.cpp qrceditor/resourcefile_p.h
    qrceditor/resourceview.cpp qrceditor/resourceview.h
    qrceditor/undocommands.cpp qrceditor/undocommands_p.h
    resource_global.h
    resourceeditorconstants.h
    resourceeditorfactory.cpp resourceeditorfactory.h
    resourceeditorplugin.cpp resourceeditorplugin.h
    resourceeditortr.h
    resourceeditorw.cpp resourceeditorw.h
    resourcenode.cpp resourcenode.h
)
