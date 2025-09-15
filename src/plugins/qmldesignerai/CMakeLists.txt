add_qtc_plugin(QmlDesignerAi
  CONDITION TARGET QtCreator::QmlDesigner
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner QtCreator::QmlDesignerBase
  DEPENDS
    Qt6::Qml
    AstCheck
  SOURCES
    qmldesigneraiplugin.cpp qmldesigneraiplugin.h
    aiassistantview.cpp aiassistantview.h
    aiassistantwidget.cpp aiassistantwidget.h
    aiassistant.qrc
    airesponse.h airesponse.cpp
    manifest.cpp manifest.h
)
