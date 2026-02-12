add_qtc_plugin(QmlDesignerMcp
  CONDITION TARGET QtCreator::QmlDesigner
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner QtCreator::QmlDesignerBase
  DEPENDS
    Qt6::Qml
    AstCheck
  SOURCES
    aiassistantconstants.h
    aiassistantview.cpp aiassistantview.h
    aiassistantwidget.cpp aiassistantwidget.h
    aiassistanttermsdialog.cpp aiassistanttermsdialog.h
    aiassistantutils.cpp aiassistantutils.h
    aiassistant.qrc
    aimodelsmodel.cpp aimodelsmodel.h
    airesponse.cpp airesponse.h
    aitransaction.cpp aitransaction.h
    manifest.cpp manifest.h
    qmldesignermcpplugin.cpp qmldesignermcpplugin.h
)

extend_qtc_plugin(QmlDesignerMcp
  PUBLIC_INCLUDES settings
  SOURCES_PREFIX settings
  SOURCES
    aiproviderconfig.cpp aiproviderconfig.h
    aiproviderdata.cpp aiproviderdata.h
    aiprovidersettings.cpp aiprovidersettings.h
    aiprovidersettingswidget.cpp aiprovidersettingswidget.h
    stringlistwidget.cpp stringlistwidget.h
)

extend_qtc_plugin(QmlDesignerMcp
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

extend_qtc_plugin(QmlDesignerMcp
  PUBLIC_INCLUDES mcp
  SOURCES_PREFIX mcp
  SOURCES
    mcpclient.cpp mcpclient.h
    mcpconfigloader.cpp mcpconfigloader.h
    mcphost.cpp mcphost.h
)

qtc_copy_to_builddir(copy_mcpserver_py_file_to_builddir
  FILES mcp/qml_mcp_server.py
  DESTINATION ${IDE_DATA_PATH}
)
