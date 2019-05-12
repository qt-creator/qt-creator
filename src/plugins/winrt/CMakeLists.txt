add_qtc_plugin(WinRt
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport
  SOURCES
    winrt.qrc
    winrtconstants.h
    winrtdebugsupport.cpp winrtdebugsupport.h
    winrtdeployconfiguration.cpp winrtdeployconfiguration.h
    winrtdevice.cpp winrtdevice.h
    winrtpackagedeploymentstep.cpp winrtpackagedeploymentstep.h
    winrtphoneqtversion.cpp winrtphoneqtversion.h
    winrtplugin.cpp winrtplugin.h
    winrtqtversion.cpp winrtqtversion.h
    winrtrunconfiguration.cpp winrtrunconfiguration.h
    winrtruncontrol.cpp winrtruncontrol.h
    winrtrunnerhelper.cpp winrtrunnerhelper.h
)
