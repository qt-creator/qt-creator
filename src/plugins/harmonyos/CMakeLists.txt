add_qtc_plugin(HarmonyOS
  PLUGIN_DEPENDS Core CMakeProjectManager Debugger ProjectExplorer QtSupport Remote
  SOURCES
    harmonyosbuilddevice.cpp harmonyosbuilddevice.h
    harmonyosconfigurations.cpp harmonyosconfigurations.h
    harmonyosdebugsupport.cpp harmonyosdebugsupport.h
    harmonyosconstants.h
    harmonyosdeploystep.cpp harmonyosdeploystep.h
    harmonyosdevice.cpp harmonyosdevice.h
    harmonyosplugin.cpp
    harmonyosqtversion.cpp harmonyosqtversion.h
    harmonyosrunconfiguration.cpp harmonyosrunconfiguration.h
    harmonyossdk.cpp harmonyossdk.h
    harmonyossettings.cpp harmonyossettings.h
    harmonyostoolchain.cpp harmonyostoolchain.h
    harmonyostr.h
)

extend_qtc_plugin(HarmonyOS
  CONDITION WITH_TESTS
  DEPENDS CmdBridgeClient
  SOURCES
    harmonyosdevice_test.cpp harmonyosdevice_test.h
)
