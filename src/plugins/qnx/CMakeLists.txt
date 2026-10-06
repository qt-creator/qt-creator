add_qtc_plugin(Qnx
  DEPENDS QmlDebug Qt::Xml
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport Remote
  SOURCES
    qnxanalyzesupport.cpp qnxanalyzesupport.h
    qnxconstants.h
    qnxdebugsupport.cpp qnxdebugsupport.h
    qnxdeployqtlibrariesdialog.cpp qnxdeployqtlibrariesdialog.h
    qnxdevice.cpp qnxdevice.h
    qnxdevicetester.cpp qnxdevicetester.h
    qnxplugin.cpp
    qnxqtversion.cpp qnxqtversion.h
    qnxrunconfiguration.cpp qnxrunconfiguration.h
    qnxtoolchain.cpp qnxtoolchain.h
    qnxtr.h
    qnxutils.cpp qnxutils.h
    slog2inforunner.cpp slog2inforunner.h
)

qtc_add_resources(Qnx "images"
  PREFIX "/qnx"
  FILES
    images/qnxdevice.png
    images/qnxdevice@2x.png
    images/qnxdevicesmall.png
    images/qnxdevicesmall@2x.png
)
