add_qtc_plugin(ExtraPropertyEditorManager
  CONDITION TARGET QtCreator::QmlDesigner AND NOT QTC_USE_QML_DESIGNER_LITE
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner QmlDesigner
  DEPENDS
    Qt::Core Qt::CorePrivate Qt6::Qml QtCreator::Utils AdvancedDockingSystem
  SOURCES
    extrapropertyeditoraction.cpp extrapropertyeditoraction.h
    extrapropertyeditormanagerplugin.cpp
    extrapropertyeditormanagerplugin.h
    extrapropertyeditorview.cpp
    extrapropertyeditorview.h
)
