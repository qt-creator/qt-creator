add_qtc_plugin(QmlDesignerAi
  CONDITION TARGET QtCreator::QmlDesigner
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner
  SOURCES
    qmldesigneraiplugin.cpp qmldesigneraiplugin.h
    aiassistantview.cpp aiassistantview.h
    aiassistantwidget.cpp aiassistantwidget.h
    aiassistant.qrc
)
