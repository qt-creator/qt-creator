import qbs 1.0

QtcPlugin {
    name: "WinRt"

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "QmakeProjectManager" }
    Depends { name: "Qt.gui" }
    Depends { name: "app_version_header" }

    files: [
        "winrtconstants.h",
        "winrtdebugsupport.cpp",
        "winrtdebugsupport.h",
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
        "winrtruncontrol.cpp",
        "winrtruncontrol.h",
        "winrtrunnerhelper.cpp",
        "winrtrunnerhelper.h"
    ]
}
