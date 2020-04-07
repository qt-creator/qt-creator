add_qtc_plugin(McuSupport
  DEPENDS Qt5::Core
  PLUGIN_DEPENDS Core ProjectExplorer Debugger CMakeProjectManager QtSupport
  SOURCES
    mcusupport.qrc
    mcusupport_global.h
    mcusupportconstants.h
    mcusupportdevice.cpp mcusupportdevice.h
    mcusupportoptions.cpp mcusupportoptions.h
    mcusupportoptionspage.cpp mcusupportoptionspage.h
    mcusupportplugin.cpp mcusupportplugin.h
    mcusupportsdk.cpp mcusupportsdk.h
    mcusupportrunconfiguration.cpp mcusupportrunconfiguration.h
)
