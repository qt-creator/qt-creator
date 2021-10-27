add_qtc_plugin(Docker
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    docker_global.h
    dockerbuildstep.cpp dockerbuildstep.h
    dockerconstants.h
    dockerdevice.cpp dockerdevice.h
    dockerplugin.cpp dockerplugin.h
    dockersettings.cpp dockersettings.h
)
