add_qtc_plugin(DevContainerPlugin
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  DEPENDS DevContainer CmdBridgeClient
  SOURCES
    devcontainerplugin.cpp
    devcontainerplugin_global.h
    devcontainerplugintr.h
    devcontainerdevice.cpp
    devcontainerdevice.h
)
