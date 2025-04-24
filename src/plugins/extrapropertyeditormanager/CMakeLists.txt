add_qtc_plugin(ExtraPropertyEditorManager
  CONDITION TARGET QtCreator::QmlDesigner AND NOT QTC_USE_QML_DESIGNER_LITE
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner
  DEPENDS
    Qt::Core Qt::CorePrivate QtCreator::Utils
  SOURCES
    extrapropertyeditormanagerplugin.cpp
    extrapropertyeditormanagerplugin.h
    extrapropertyeditorview.cpp
    extrapropertyeditorview.h
)
