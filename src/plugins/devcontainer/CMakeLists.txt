add_qtc_plugin(DevContainerPlugin
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  PLUGIN_TEST_DEPENDS CMakeProjectManager
  DEPENDS DevContainer CmdBridgeClient
  SOURCES
    devcontainerplugin.cpp
    devcontainerplugin_global.h
    devcontainerplugintr.h
    devcontainerdevice.cpp
    devcontainerdevice.h
)


extend_qtc_plugin(DevContainerPlugin
  CONDITION WITH_TESTS
  DEFINES TESTDATA="${PROJECT_BINARY_DIR}/${IDE_DATA_PATH}/devcontainer_testdata"
  SOURCES
    devcontainer_test.cpp
)

qtc_copy_to_builddir(copy_devcontainerplugin_testdata
  DIRECTORIES testdata
  DESTINATION "${IDE_DATA_PATH}/devcontainer_testdata")
