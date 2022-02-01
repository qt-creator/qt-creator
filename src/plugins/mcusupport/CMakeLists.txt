add_qtc_plugin(McuSupport
  DEPENDS Qt5::Core
  PLUGIN_DEPENDS Core BareMetal ProjectExplorer Debugger CMakeProjectManager QtSupport
  SOURCES
    mcusupport.qrc
    mcusupport_global.h
    mcusupportconstants.h
    mcusupportdevice.cpp mcusupportdevice.h
    mcusupportoptions.cpp mcusupportoptions.h
    mcusupportoptionspage.cpp mcusupportoptionspage.h
    mcupackage.cpp mcupackage.h
    mcusupportplugin.cpp mcusupportplugin.h
    mcusupportsdk.cpp mcusupportsdk.h
    mcusupportrunconfiguration.cpp mcusupportrunconfiguration.h
    mcusupportversiondetection.cpp mcusupportversiondetection.h
    mcusupportcmakemapper.cpp mcusupportcmakemapper.h
)
