# TODO: unittest should be fixed
if(APPLE)
  set(plugin_output_name "libtest_debug")
elseif(UNIX)
  set(plugin_output_name "libtest")
else()
  set(plugin_output_name "test$<$<CONFIG:Debug>:d>")
endif()

file(RELATIVE_PATH TEST_PLUGIN_PATH ${QtCreator_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_qtc_plugin(testplugin
  DEPENDS ExtensionSystem
  DEFINES MYPLUGIN_LIBRARY
  SOURCES
    testplugin.cpp testplugin.h
  PLUGIN_PATH "${TEST_PLUGIN_PATH}"
  SKIP_INSTALL
  INTERNAL_ONLY
)

# The empty string gets removed if I put it above
if (TARGET testplugin)
  set_target_properties(testplugin PROPERTIES PREFIX ""
                                              OUTPUT_NAME "${plugin_output_name}")
endif()
