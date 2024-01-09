find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_qtc_plugin(QtApplicationManagerIntegration
  PLUGIN_DEPENDS
    Core Debugger ProjectExplorer QmakeProjectManager
    QtSupport RemoteLinux
  DEPENDS Qt::Network Qt::Widgets ExtensionSystem Utils
  SOURCES
    appmanagerconstants.h
    appmanagercreatepackagestep.cpp appmanagercreatepackagestep.h
    appmanagerdeployconfigurationautoswitcher.cpp appmanagerdeployconfigurationautoswitcher.h
    appmanagerdeployconfigurationfactory.cpp appmanagerdeployconfigurationfactory.h
    appmanagerdeploypackagestep.cpp appmanagerdeploypackagestep.h
    appmanagerinstallpackagestep.cpp appmanagerinstallpackagestep.h
    appmanagermakeinstallstep.cpp appmanagermakeinstallstep.h
    appmanagerplugin.cpp appmanagerplugin.h
    appmanagerrunconfiguration.cpp appmanagerrunconfiguration.h
    appmanagerruncontrol.cpp appmanagerruncontrol.h
    appmanagerstringaspect.cpp appmanagerstringaspect.h
    appmanagertargetinformation.cpp appmanagertargetinformation.h
    appmanagertr.h
    appmanagerutilities.cpp appmanagerutilities.h
)
