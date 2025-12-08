add_qtc_plugin(QmlDesignerAi
  CONDITION TARGET QtCreator::QmlDesigner
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner QtCreator::QmlDesignerBase
  DEPENDS
    Qt6::Qml
    AstCheck
  SOURCES
    aiassistantconstants.h
    qmldesigneraiplugin.cpp qmldesigneraiplugin.h
    aiassistantview.cpp aiassistantview.h
    aiassistantwidget.cpp aiassistantwidget.h
    aiassistanttermsdialog.cpp aiassistanttermsdialog.h
    aiassistantutils.cpp aiassistantutils.h
    aiassistant.qrc
    aimodelsmodel.cpp aimodelsmodel.h
    airesponse.cpp airesponse.h
    aitransaction.cpp aitransaction.h
    manifest.cpp manifest.h
)

extend_qtc_plugin(QmlDesignerAi
  PUBLIC_INCLUDES settings
  SOURCES_PREFIX settings
  SOURCES
    aiproviderconfig.cpp aiproviderconfig.h
    aiproviderdata.cpp aiproviderdata.h
    aiprovidersettings.cpp aiprovidersettings.h
    aiprovidersettingswidget.cpp aiprovidersettingswidget.h
    stringlistwidget.cpp stringlistwidget.h
)

extend_qtc_plugin(QmlDesignerAi
  PUBLIC_INCLUDES api
  SOURCES_PREFIX api
  SOURCES
    abstractaiapi.h
    aiapimanager.cpp aiapimanager.h
    aiapiutils.cpp aiapiutils.h
    claudemessagesapi.cpp claudemessagesapi.h
    openaicompletionsapi.cpp openaicompletionsapi.h
    openairesponsesapi.cpp openairesponsesapi.h
)
