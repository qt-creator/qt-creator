add_qtc_plugin(Ios
  DEPENDS QmlDebug Qt::Xml
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QmakeProjectManager CMakeProjectManager
  SOURCES
    devicectlutils.cpp
    devicectlutils.h
    ios.qrc
    iosbuildconfiguration.cpp iosbuildconfiguration.h
    iosbuildstep.cpp iosbuildstep.h
    iosconfigurations.cpp iosconfigurations.h
    iosconstants.h
    iosdeploystep.cpp iosdeploystep.h
    iosdevice.cpp iosdevice.h
    iosdsymbuildstep.cpp iosdsymbuildstep.h
    iosplugin.cpp
    iosprobe.cpp iosprobe.h
    iosqtversion.cpp iosqtversion.h
    iosrunconfiguration.cpp iosrunconfiguration.h
    iosrunner.cpp iosrunner.h
    iossettingspage.cpp iossettingspage.h
    iossimulator.cpp iossimulator.h
    iostoolhandler.cpp iostoolhandler.h
    iostr.h
    simulatorcontrol.cpp simulatorcontrol.h
)

extend_qtc_plugin(Ios
  CONDITION APPLE
  DEPENDS ${FWCoreFoundation} ${FWIOKit}
)
