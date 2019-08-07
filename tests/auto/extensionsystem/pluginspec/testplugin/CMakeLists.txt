# TODO: unittest should be fixed
if(APPLE)
  set(plugin_output_name "libtest_debug")
elseif(UNIX)
  set(plugin_output_name "liblibtest")
else()
  set(plugin_output_name "testd4")
endif()

file(RELATIVE_PATH TEST_PLUGIN_PATH ${QtCreator_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_qtc_plugin(testplugin
  DEPENDS ExtensionSystem
  DEFINES MYPLUGIN_LIBRARY
  SOURCES
    testplugin.cpp testplugin.h
  PLUGIN_PATH "${TEST_PLUGIN_PATH}"
  PROPERTIES
    OUTPUT_NAME ${plugin_output_name}
  SKIP_INSTALL
  INTERNAL_ONLY
)

# The empty string gets removed if I put it above
set_target_properties(testplugin PROPERTIES PREFIX "")
