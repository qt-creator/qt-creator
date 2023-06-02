add_qtc_plugin(Conan
  PLUGIN_DEPENDS Core ProjectExplorer
  SOURCES
    conanconstants.h
    conaninstallstep.cpp conaninstallstep.h
    conanplugin.cpp
    conansettings.cpp conansettings.h
    conantr.h
)
