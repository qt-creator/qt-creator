add_subdirectory(testplugin)

add_qtc_test(tst_pluginspec
  DEFINES
    PLUGIN_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    PLUGINSPEC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    DLL_INFIX="$<$<CONFIG:Debug>:d>"
  DEPENDS ExtensionSystem
  SOURCES
    tst_pluginspec.cpp
)
