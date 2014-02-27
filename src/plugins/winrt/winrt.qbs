import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "WinRt"
    minimumQtVersion: "5.0"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "Qt.gui" }

    files: [
        "winrtconstants.h",
        "winrtdeployconfiguration.cpp",
        "winrtdeployconfiguration.h",
        "winrtdevice.cpp",
        "winrtdevice.h",
        "winrtdevicefactory.cpp",
        "winrtdevicefactory.h",
        "winrtpackagedeploymentstep.cpp",
        "winrtpackagedeploymentstep.h",
        "winrtphoneqtversion.cpp",
        "winrtphoneqtversion.h",
        "winrtplugin.cpp",
        "winrtplugin.h",
        "winrtqtversion.cpp",
        "winrtqtversion.h",
        "winrtqtversionfactory.cpp",
        "winrtqtversionfactory.h",
        "winrtrunconfiguration.cpp",
        "winrtrunconfiguration.h",
        "winrtrunconfigurationwidget.cpp",
        "winrtrunconfigurationwidget.h",
        "winrtrunconfigurationwidget.ui",
        "winrtruncontrol.cpp",
        "winrtruncontrol.h",
        "winrtrunfactories.cpp",
        "winrtrunfactories.h"
    ]
}
