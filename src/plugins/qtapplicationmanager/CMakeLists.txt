find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_qtc_plugin(QtApplicationManagerIntegration
  PLUGIN_DEPENDS
    Core Debugger ProjectExplorer
    QtSupport RemoteLinux
  DEPENDS Qt::Network Qt::Widgets ExtensionSystem Utils yaml-cpp
  SOURCES
    appmanagerconstants.h
    appmanagercreatepackagestep.cpp appmanagercreatepackagestep.h
    appmanagerdeployconfigurationautoswitcher.cpp appmanagerdeployconfigurationautoswitcher.h
    appmanagerdeployconfigurationfactory.cpp appmanagerdeployconfigurationfactory.h
    appmanagerdeploypackagestep.cpp appmanagerdeploypackagestep.h
    appmanagerinstallpackagestep.cpp appmanagerinstallpackagestep.h
    appmanagerremoteinstallpackagestep.cpp appmanagerremoteinstallpackagestep.h
    appmanagermakeinstallstep.cpp appmanagermakeinstallstep.h
    appmanagercmakepackagestep.cpp appmanagercmakepackagestep.h
    appmanagerplugin.cpp
    appmanagerrunconfiguration.cpp appmanagerrunconfiguration.h
    appmanagerruncontrol.cpp appmanagerruncontrol.h
    appmanagerstringaspect.cpp appmanagerstringaspect.h
    appmanagertargetinformation.cpp appmanagertargetinformation.h
    appmanagertr.h
    appmanagerutilities.cpp appmanagerutilities.h
)
