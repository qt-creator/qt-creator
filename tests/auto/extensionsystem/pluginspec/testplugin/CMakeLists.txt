# TODO: unittest should be fixed
if(APPLE)
  set(plugin_output_name "libtest_debug")
elseif(UNIX)
  set(plugin_output_name "liblibtest")
else()
  set(plugin_output_name "testd4")
endif()

add_qtc_plugin(testplugin
  DEPENDS ExtensionSystem
  DEFINES MYPLUGIN_LIBRARY
  SOURCES
    testplugin.cpp testplugin.h
  PLUGIN_PATH "${CMAKE_CURRENT_BINARY_DIR}"
  PROPERTIES
    OUTPUT_NAME ${plugin_output_name}
  SKIP_INSTALL
)

# The empty string gets removed if I put it above
set_target_properties(testplugin PROPERTIES PREFIX "")
