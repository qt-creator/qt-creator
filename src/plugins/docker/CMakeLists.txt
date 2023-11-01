add_qtc_plugin(Docker
  DEPENDS Utils
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    docker_global.h
    dockertr.h
    dockerapi.cpp dockerapi.h
    dockerconstants.h
    dockerdevice.cpp dockerdevice.h
    dockerdevicewidget.cpp dockerdevicewidget.h
    dockerplugin.cpp
    dockersettings.cpp dockersettings.h
    kitdetector.cpp kitdetector.h
)
