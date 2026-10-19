add_qtc_plugin(ExtraPropertyEditorManager
  CONDITION TARGET QtCreator::QmlDesigner
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner
  DEPENDS
    Qt::Core QtCreator::Utils
  SOURCES
    extrapropertyeditormanagerplugin.cpp
    extrapropertyeditormanagerplugin.h
    extrapropertyeditorview.cpp
    extrapropertyeditorview.h
)
