if(BUILD_DESIGNSTUDIO AND ($<CONFIG:Debug> OR WITH_TESTS))
  set(ENABLE_COMPILE_WARNING_AS_ERROR_DEFAULT ON)
else()
  set(ENABLE_COMPILE_WARNING_AS_ERROR_DEFAULT OFF)
endif()
env_with_default("QDS_ENABLE_COMPILE_WARNING_AS_ERROR" ENV_ENABLE_COMPILE_WARNING_AS_ERROR
                 ${ENABLE_COMPILE_WARNING_AS_ERROR_DEFAULT})
option(ENABLE_COMPILE_WARNING_AS_ERROR "Treat warnings as errors in QmlDesigner" ${ENV_ENABLE_COMPILE_WARNING_AS_ERROR})
add_feature_info("Treat warnings as errors in QmlDesigner" ${ENABLE_COMPILE_WARNING_AS_ERROR} "")

add_qtc_library(QmlDesignerSettings STATIC
  DEPENDS Utils
  PUBLIC_INCLUDES settings
  SOURCES_PREFIX settings
  SOURCES
    qmldesignersettings_global.h
    designersettings.cpp designersettings.h
)

add_qtc_plugin(QmlDesignerBase
  CONDITION TARGET Qt::QuickWidgets
  DEPENDS Qt::Core Qt::QuickWidgets
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  PUBLIC_INCLUDES settings
  SOURCES
    qmldesignerbase_global.h
    qmldesignerbaseplugin.cpp qmldesignerbaseplugin.h
)

extend_qtc_plugin(QmlDesignerBase
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
)

extend_qtc_plugin(QmlDesignerBase
  PUBLIC_INCLUDES utils
  SOURCES_PREFIX utils
  SOURCES
    designerpaths.cpp designerpaths.h
    qmlpuppetpaths.cpp qmlpuppetpaths.h
    windowmanager.cpp windowmanager.h
)

extend_qtc_plugin(QmlDesignerBase
  PUBLIC_INCLUDES studio
  SOURCES_PREFIX studio
  SOURCES
    studiostyle.cpp studiostyle.h
    studiostyle_p.cpp studiostyle_p.h
    studioquickwidget.cpp studioquickwidget.h
    studiosettingspage.cpp studiosettingspage.h
)


# from cmake 3.24 $<LINK_LIBRARY:WHOLE_ARCHIVE,QmlDesignerSettings> is supported which can be added to DEPENDS
if (MSVC)
  qtc_output_binary_dir(_output_binary_dir)
  set_target_properties(QmlDesignerBase PROPERTIES LINK_FLAGS "/WHOLEARCHIVE:${_output_binary_dir}/${IDE_BIN_PATH}/QmlDesignerSettings.lib")
elseif (APPLE)
  target_link_libraries(QmlDesignerBase PRIVATE -Wl,-force_load $<TARGET_LINKER_FILE:QmlDesignerSettings>)
else ()
  target_link_libraries(QmlDesignerBase PRIVATE -Wl,--whole-archive $<TARGET_LINKER_FILE:QmlDesignerSettings> -Wl,--no-whole-archive)
endif ()
