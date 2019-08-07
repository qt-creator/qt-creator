add_qtc_plugin(plugin3
  DEPENDS ExtensionSystem
  SOURCES
    plugin3.cpp plugin3.h
  PLUGIN_PATH "${TEST_PLUGIN_PATH}"
  SKIP_INSTALL
  INTERNAL_ONLY
)
