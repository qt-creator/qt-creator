add_qtc_plugin(DevContainerPlugin
  PLUGIN_DEPENDS Core ProjectExplorer TextEditor
  PLUGIN_TEST_DEPENDS CMakeProjectManager
  DEPENDS DevContainer CmdBridgeClient
  LONG_DESCRIPTION_MD README.md
  SOURCES
    devcontainerplugin.cpp
    devcontainerplugin_constants.h
    devcontainerplugin_global.h
    devcontainerplugintr.h
    devcontainerdevice.cpp
    devcontainerdevice.h
)

qtc_add_resources(DevContainerPlugin "images"
  PREFIX "/devcontainer"
  BASE "."
  FILES
    images/container.png
    images/container@2x.png
    images/containerdevice.png
    images/containerdevice@2x.png
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
