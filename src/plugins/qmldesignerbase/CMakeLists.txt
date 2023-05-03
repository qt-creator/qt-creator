if(BUILD_DESIGNSTUDIO AND ($<CONFIG:Debug> OR WITH_TESTS))
  set(ENABLE_COMPILE_WARNING_AS_ERROR_DEFAULT ON)
else()
  set(ENABLE_COMPILE_WARNING_AS_ERROR_DEFAULT OFF)
endif()
env_with_default("QDS_ENABLE_COMPILE_WARNING_AS_ERROR" ENV_ENABLE_COMPILE_WARNING_AS_ERROR
                 ${ENABLE_COMPILE_WARNING_AS_ERROR_DEFAULT})
option(ENABLE_COMPILE_WARNING_AS_ERROR "Treat warnings as errors in QmlDesigner" ${ENV_ENABLE_COMPILE_WARNING_AS_ERROR})
add_feature_info("Treat warnings as errors in QmlDesigner" ${ENABLE_COMPILE_WARNING_AS_ERROR} "")

add_qtc_plugin(QmlDesignerBase
  CONDITION TARGET Qt::QuickWidgets
  DEPENDS Qt::Core Qt::QuickWidgets
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    qmldesignerbase_global.h
    qmldesignerbaseplugin.cpp qmldesignerbaseplugin.h
)

extend_qtc_plugin(QmlDesignerBase
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
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
    studiostyle_p.cpp studiostyle_p.h
    studioquickwidget.cpp studioquickwidget.h
    studiosettingspage.cpp studiosettingspage.h
)
