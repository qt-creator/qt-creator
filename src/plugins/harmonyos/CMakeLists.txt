add_qtc_plugin(HarmonyOS
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport
  SOURCES
    harmonyosconfigurations.cpp harmonyosconfigurations.h
    harmonyosconstants.h
    harmonyosdevice.cpp harmonyosdevice.h
    harmonyosplugin.cpp
    harmonyosqtversion.cpp harmonyosqtversion.h
    harmonyossdk.cpp harmonyossdk.h
    harmonyossettings.cpp harmonyossettings.h
    harmonyostoolchain.cpp harmonyostoolchain.h
    harmonyostr.h
)
