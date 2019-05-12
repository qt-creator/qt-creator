add_subdirectory(circularplugins)
add_subdirectory(correctplugins1)

add_qtc_test(tst_pluginmanager
  DEFINES PLUGINMANAGER_TESTS_DIR="${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS ExtensionSystem
  SOURCES
    tst_pluginmanager.cpp
)
