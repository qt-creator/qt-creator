add_qtc_plugin(Conan
  PLUGIN_DEPENDS Core ProjectExplorer
  SOURCES
    conaninstallstep.cpp conaninstallstep.h
    conanplugin.cpp conanplugin.h
    conansettings.cpp conansettings.h
)
