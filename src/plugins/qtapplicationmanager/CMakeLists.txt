find_package(yaml-cpp QUIET MODULE)

add_qtc_plugin(QtApplicationManagerIntegration
  CONDITION TARGET yaml-cpp::yaml-cpp
  PLUGIN_DEPENDS
    Core Debugger ProjectExplorer
    QtSupport RemoteLinux
  PLUGIN_CLASS AppManagerPlugin
  DEPENDS Qt::Network Qt::Widgets ExtensionSystem Utils yaml-cpp::yaml-cpp
  SOURCES
    appmanagerconstants.h
    appmanagercreatepackagestep.cpp appmanagercreatepackagestep.h
    appmanagerdeployconfigurationautoswitcher.cpp appmanagerdeployconfigurationautoswitcher.h
    appmanagerdeployconfigurationfactory.cpp appmanagerdeployconfigurationfactory.h
    appmanagerdeploypackagestep.cpp appmanagerdeploypackagestep.h
    appmanagerinstallpackagestep.cpp appmanagerinstallpackagestep.h
    appmanagercmakepackagestep.cpp appmanagercmakepackagestep.h
    appmanagerplugin.cpp
    appmanagerrunconfiguration.cpp appmanagerrunconfiguration.h
    appmanagerruncontrol.cpp appmanagerruncontrol.h
    appmanagerstringaspect.cpp appmanagerstringaspect.h
    appmanagertargetinformation.cpp appmanagertargetinformation.h
    appmanagertr.h
    appmanagerutilities.cpp appmanagerutilities.h
)
