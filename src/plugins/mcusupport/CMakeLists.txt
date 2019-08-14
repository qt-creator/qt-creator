add_qtc_plugin(McuSupport
  DEPENDS Qt5::Core
  PLUGIN_DEPENDS Core ProjectExplorer CMakeProjectManager
  SOURCES
    mcusupport.qrc
    mcusupport_global.h
    mcusupportconstants.h
    mcusupportdevice.cpp mcusupportdevice.h
    mcusupportoptionspage.cpp mcusupportoptionspage.h
    mcusupportplugin.cpp mcusupportplugin.h
    mcusupportrunconfiguration.cpp mcusupportrunconfiguration.h
)
