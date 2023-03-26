add_qtc_plugin(Qnx
  DEPENDS QmlDebug Qt::Xml
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport RemoteLinux
  SOURCES
    qnx.qrc
    qnxanalyzesupport.cpp qnxanalyzesupport.h
    qnxconfiguration.cpp qnxconfiguration.h
    qnxconfigurationmanager.cpp qnxconfigurationmanager.h
    qnxconstants.h
    qnxdebugsupport.cpp qnxdebugsupport.h
    qnxdeployqtlibrariesdialog.cpp qnxdeployqtlibrariesdialog.h
    qnxdevice.cpp qnxdevice.h
    qnxdeviceprocesslist.cpp qnxdeviceprocesslist.h
    qnxdeviceprocesssignaloperation.cpp qnxdeviceprocesssignaloperation.h
    qnxdevicetester.cpp qnxdevicetester.h
    qnxdevicewizard.cpp qnxdevicewizard.h
    qnxplugin.cpp
    qnxqtversion.cpp qnxqtversion.h
    qnxrunconfiguration.cpp qnxrunconfiguration.h
    qnxsettingspage.cpp qnxsettingspage.h
    qnxtoolchain.cpp qnxtoolchain.h
    qnxutils.cpp qnxutils.h
    qnxversionnumber.cpp qnxversionnumber.h
    slog2inforunner.cpp slog2inforunner.h
)
