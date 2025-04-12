add_qtc_plugin(correct_plugin3
  PLUGIN_NAME plugin3
  DEPENDS ExtensionSystem Utils
  SOURCES
    plugin3.cpp plugin3.h
  PLUGIN_PATH "${TEST_PLUGIN_PATH}"
  SKIP_INSTALL
  INTERNAL_ONLY
)
