env_with_default("QDS_DISABLE_COMPILE_WARNING_AS_ERROR" ENV_QDS_DISABLE_COMPILE_WARNING_AS_ERROR OFF)
option(DISABLE_COMPILE_WARNING_AS_ERROR "Dont treat warnings as errors" ${ENV_QDS_DISABLE_COMPILE_WARNING_AS_ERROR})
add_feature_info("Treat warnings as errors" ${DISABLE_COMPILE_WARNING_AS_ERROR} "")

add_qtc_plugin(QmlDesignerBase
  CONDITION TARGET Qt::QuickWidgets
  DEPENDS Qt::Core Qt::QuickWidgets
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    qmldesignerbase_global.h
    qmldesignerbaseplugin.cpp qmldesignerbaseplugin.h
)

extend_qtc_plugin(QmlDesignerBase
    CONDITION (NOT DISABLE_COMPILE_WARNING_AS_ERROR) AND ($<CONFIG:Debug> OR WITH_TESTS)
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
)

extend_qtc_plugin(QmlDesignerBase
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/utils
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/utils
  SOURCES
    designerpaths.cpp designerpaths.h
    designersettings.cpp designersettings.h
    qmlpuppetpaths.cpp qmlpuppetpaths.h
)

extend_qtc_plugin(QmlDesignerBase
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/studio
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/studio
  SOURCES
    studiostyle.cpp studiostyle.h
    studioquickwidget.cpp studioquickwidget.h
    studiosettingspage.cpp studiosettingspage.h
)
