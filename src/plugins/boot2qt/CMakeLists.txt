add_qtc_plugin(Boot2Qt
  DEPENDS Qt5::Network QtcSsh
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport RemoteLinux
  SOURCES
    qdb.qrc
    qdb_global.h
    qdbconstants.h
    qdbdeployconfigurationfactory.cpp qdbdeployconfigurationfactory.h
    qdbdevice.cpp qdbdevice.h
    qdbdevicedebugsupport.cpp qdbdevicedebugsupport.h
    qdbmakedefaultappservice.cpp qdbmakedefaultappservice.h
    qdbmakedefaultappstep.cpp qdbmakedefaultappstep.h
    qdbplugin.cpp qdbplugin.h
    qdbqtversion.cpp qdbqtversion.h
    qdbrunconfiguration.cpp qdbrunconfiguration.h
    qdbstopapplicationservice.cpp qdbstopapplicationservice.h
    qdbstopapplicationstep.cpp qdbstopapplicationstep.h
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

