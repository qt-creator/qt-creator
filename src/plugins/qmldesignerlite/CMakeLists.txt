if(BUILD_DESIGNSTUDIO AND ($<CONFIG:Debug> OR WITH_TESTS))
  set(ENABLE_COMPILE_WARNING_AS_ERROR_DEFAULT ON)
else()
  set(ENABLE_COMPILE_WARNING_AS_ERROR_DEFAULT OFF)
endif()
env_with_default("QDS_ENABLE_COMPILE_WARNING_AS_ERROR" ENV_ENABLE_COMPILE_WARNING_AS_ERROR
                 ${ENABLE_COMPILE_WARNING_AS_ERROR_DEFAULT})
option(ENABLE_COMPILE_WARNING_AS_ERROR "Treat warnings as errors in QmlDesigner" ${ENV_ENABLE_COMPILE_WARNING_AS_ERROR})
add_feature_info("Treat warnings as errors in QmlDesigner" ${ENABLE_COMPILE_WARNING_AS_ERROR} "")

add_qtc_plugin(QmlDesignerLite
  PLUGIN_DEPENDS QmlDesigner
  SOURCES
    qmldesignerliteplugin.cpp qmldesignerliteplugin.h
)

extend_qtc_plugin(QmlDesignerBase
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
)
