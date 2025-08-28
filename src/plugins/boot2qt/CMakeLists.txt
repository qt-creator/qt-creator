add_qtc_plugin(Boot2Qt
  PLUGIN_CLASS QdbPlugin
  DEPENDS Qt::Network
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport RemoteLinux
  SOURCES
    qdb_global.h
    qdbconstants.h
    qdbdevice.cpp qdbdevice.h
    qdbdevicedebugsupport.cpp qdbdevicedebugsupport.h
    qdbmakedefaultappstep.cpp qdbmakedefaultappstep.h
    qdbplugin.cpp
    qdbqtversion.cpp qdbqtversion.h
    qdbrunconfiguration.cpp qdbrunconfiguration.h
    qdbstopapplicationstep.cpp qdbstopapplicationstep.h
    qdbtr.h
    qdbutils.cpp qdbutils.h
)

extend_qtc_plugin(Boot2Qt
  SOURCES_PREFIX device-detection
  SOURCES
    devicedetector.cpp devicedetector.h
    hostmessages.cpp hostmessages.h
    qdbdevicetracker.cpp qdbdevicetracker.h
    qdbmessagetracker.cpp qdbmessagetracker.h
    qdbwatcher.cpp qdbwatcher.h
)

qtc_add_resources(Boot2Qt "images"
  PREFIX "/qdb"
  BASE "."
  FILES
    images/qdbdevice.png
    images/qdbdevice@2x.png
    images/qdbdevicesmall.png
    images/qdbdevicesmall@2x.png
)
