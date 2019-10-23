add_qtc_plugin(McuSupport
  DEPENDS Qt5::Core
  PLUGIN_DEPENDS Core ProjectExplorer Debugger CMakeProjectManager
  SOURCES
    mcusupport.qrc
    mcusupport_global.h
    mcusupportconstants.h
    mcusupportdevice.cpp mcusupportdevice.h
    mcusupportoptions.cpp mcusupportoptions.h
    mcusupportoptionspage.cpp mcusupportoptionspage.h
    mcusupportplugin.cpp mcusupportplugin.h
    mcusupportrunconfiguration.cpp mcusupportrunconfiguration.h
)
