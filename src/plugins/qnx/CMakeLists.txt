add_qtc_plugin(Qnx
  DEPENDS QtcSsh QmlDebug Qt5::Xml
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport RemoteLinux
  SOURCES
    qnx.qrc
    qnx_export.h
    qnxanalyzesupport.cpp qnxanalyzesupport.h
    qnxbaseqtconfigwidget.cpp qnxbaseqtconfigwidget.h
    qnxconfiguration.cpp qnxconfiguration.h
    qnxconfigurationmanager.cpp qnxconfigurationmanager.h
    qnxconstants.h
    qnxdebugsupport.cpp qnxdebugsupport.h
    qnxdeployqtlibrariesdialog.cpp qnxdeployqtlibrariesdialog.h qnxdeployqtlibrariesdialog.ui
    qnxdevice.cpp qnxdevice.h
    qnxdeviceprocess.cpp qnxdeviceprocess.h
    qnxdeviceprocesslist.cpp qnxdeviceprocesslist.h
    qnxdeviceprocesssignaloperation.cpp qnxdeviceprocesssignaloperation.h
    qnxdevicetester.cpp qnxdevicetester.h
    qnxdevicewizard.cpp qnxdevicewizard.h
    qnxplugin.cpp qnxplugin.h
    qnxqtversion.cpp qnxqtversion.h
    qnxrunconfiguration.cpp qnxrunconfiguration.h
    qnxsettingspage.cpp qnxsettingspage.h
    qnxsettingswidget.ui
    qnxtoolchain.cpp qnxtoolchain.h
    qnxutils.cpp qnxutils.h
    qnxversionnumber.cpp qnxversionnumber.h
    slog2inforunner.cpp slog2inforunner.h
)
