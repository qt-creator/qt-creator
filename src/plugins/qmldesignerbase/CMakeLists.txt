add_qtc_plugin(QmlDesignerBase
  DEPENDS Qt::Core
  PLUGIN_DEPENDS Core ProjectExplorer
  SOURCES
    qmldesignerbase_global.h
    qmldesignerbaseplugin.cpp qmldesignerbaseplugin.h
)

extend_qtc_plugin(QmlDesignerBase
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/utils
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/utils
  SOURCES
    designersettings.cpp designersettings.h
)
