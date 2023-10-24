add_qtc_plugin(McuSupport
  DEPENDS Qt::Core QmlJS
  PLUGIN_DEPENDS Core BareMetal ProjectExplorer Debugger CMakeProjectManager QmlJSTools QtSupport
  SOURCES
    mcukitaspect.cpp mcukitaspect.h
    mcusupport.qrc
    mcusupport_global.h
    mcusupporttr.h
    mcusupportconstants.h
    mcusupportdevice.cpp mcusupportdevice.h
    mcusupportoptions.cpp mcusupportoptions.h mcuabstractpackage.h
    mcukitmanager.cpp mcukitmanager.h
    mcusupportoptionspage.cpp mcusupportoptionspage.h
    mcupackage.cpp mcupackage.h
    mcutarget.cpp mcutarget.h
    mcutargetfactorylegacy.cpp mcutargetfactorylegacy.h
    mcutargetfactory.cpp mcutargetfactory.h
    mcusupportplugin.cpp mcusupportplugin.h
    mcusupportsdk.cpp mcusupportsdk.h
    mcusupportrunconfiguration.cpp mcusupportrunconfiguration.h
    mcusupportversiondetection.cpp mcusupportversiondetection.h
    mcutargetdescription.h
    mcuhelpers.cpp mcuhelpers.h
    settingshandler.cpp settingshandler.h
    mcuqmlprojectnode.cpp mcuqmlprojectnode.h
    mcubuildstep.cpp mcubuildstep.h
    dialogs/mcukitcreationdialog.cpp dialogs/mcukitcreationdialog.h
    mcusupportimportprovider.cpp mcusupportimportprovider.h
)

add_subdirectory(test)
