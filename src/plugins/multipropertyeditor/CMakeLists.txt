add_qtc_plugin(MultiPropertyEditor
  CONDITION TARGET QtCreator::QmlDesigner AND NOT QTC_USE_QML_DESIGNER_LITE
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner QmlDesigner
  DEPENDS
    Qt::Core Qt::CorePrivate Qt6::Qml QtCreator::Utils AdvancedDockingSystem
  SOURCES
    multipropertyeditoraction.cpp multipropertyeditoraction.h
    multipropertyeditorplugin.cpp
    multipropertyeditorplugin.h
    multipropertyeditorview.cpp
    multipropertyeditorview.h
)
