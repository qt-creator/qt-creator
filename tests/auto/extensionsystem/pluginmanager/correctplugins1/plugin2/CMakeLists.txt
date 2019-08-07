add_qtc_plugin(correct_plugin2
  PLUGIN_NAME plugin2
  DEPENDS ExtensionSystem
  SOURCES
    plugin2.cpp plugin2.h
  PLUGIN_PATH "${TEST_PLUGIN_PATH}"
  SKIP_INSTALL
  INTERNAL_ONLY
)
