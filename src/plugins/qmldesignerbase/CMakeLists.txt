add_qtc_plugin(QmlDesignerBase
  PROPERTIES COMPILE_WARNING_AS_ERROR ON
  DEPENDS Qt::Core Qt::QuickWidgets
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    qmldesignerbase_global.h
    qmldesignerbaseplugin.cpp qmldesignerbaseplugin.h
)

extend_qtc_plugin(QmlDesignerBase
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/utils
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/utils
  SOURCES
    designersettings.cpp designersettings.h
    qmlpuppetpaths.cpp qmlpuppetpaths.h
    studioquickwidget.cpp studioquickwidget.h
)
