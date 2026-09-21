add_qtc_plugin(Wsl
  DEPENDS Utils CmdBridgeClient
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    wslapi.cpp wslapi.h
    wslconstants.h
    wsldevice.cpp wsldevice.h
    wsldevicewidget.cpp wsldevicewidget.h
    wslplugin.cpp
    wsltr.h
)

add_qtc_test(tst_wsl_api
  CONDITION WITH_TESTS
  DEPENDS Utils
  SOURCES
    tests/tst_wsl_api.cpp
    wslapi.cpp
)
