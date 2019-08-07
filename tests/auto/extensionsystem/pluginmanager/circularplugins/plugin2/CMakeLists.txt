add_qtc_plugin(plugin2
  DEPENDS ExtensionSystem
  SOURCES
    plugin2.cpp plugin2.h
  PLUGIN_PATH "${TEST_PLUGIN_PATH}"
  SKIP_INSTALL
  INTERNAL_ONLY
)
