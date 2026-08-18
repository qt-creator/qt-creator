add_qtc_plugin(Docker
  DEPENDS Utils CmdBridgeClient
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  PLUGIN_TEST_DEPENDS Debugger
  SOURCES
    docker_global.h
    dockertr.h
    dockerapi.cpp dockerapi.h
    dockerconstants.h
    dockercontainerthread.cpp
    dockercontainerthread.h
    dockerdevice.cpp dockerdevice.h
    dockerdevicewidget.cpp dockerdevicewidget.h
    dockerdeviceenvironmentaspect.cpp dockerdeviceenvironmentaspect.h
    dockerplugin.cpp
    dockersettings.cpp dockersettings.h
)

extend_qtc_plugin(Docker
  CONDITION WITH_TESTS
  SOURCES
    dockerdebuggertest.h
    dockerdebuggertest.cpp
    dockermounttest.h
    dockermounttest.cpp
)

qtc_add_resources(Docker "images"
  PREFIX "/docker"
  BASE "."
  FILES
    images/dockerdevice.png
    images/dockerdevice@2x.png
    images/dockerdevicesmall.png
    images/dockerdevicesmall@2x.png
)
