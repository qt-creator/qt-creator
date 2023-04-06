add_qtc_plugin(Qnx
  DEPENDS QmlDebug Qt::Xml
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport RemoteLinux
  SOURCES
    qnx.qrc
    qnxanalyzesupport.cpp qnxanalyzesupport.h
    qnxconstants.h
    qnxdebugsupport.cpp qnxdebugsupport.h
    qnxdeployqtlibrariesdialog.cpp qnxdeployqtlibrariesdialog.h
    qnxdevice.cpp qnxdevice.h
    qnxdevicetester.cpp qnxdevicetester.h
    qnxplugin.cpp
    qnxqtversion.cpp qnxqtversion.h
    qnxrunconfiguration.cpp qnxrunconfiguration.h
    qnxsettingspage.cpp qnxsettingspage.h
    qnxtoolchain.cpp qnxtoolchain.h
    qnxtr.h
    qnxutils.cpp qnxutils.h
    qnxversionnumber.cpp qnxversionnumber.h
    slog2inforunner.cpp slog2inforunner.h
)
